/*
 * mpu6050.c
 *
 * STM32 HAL tabanli MPU6050 (ivmeolcer + jiroskop) surucusu
 * Register haritasi ve olceklendirme katsayilari InvenSense resmi
 * MPU-6000/MPU-6050 register haritasi belgesinden alinmistir.
 */

#include "mpu6050.h"

static I2C_HandleTypeDef *mpu_i2c;
static float accel_lsb_per_g;
static float gyro_lsb_per_dps;

#define I2C_TIMEOUT 100

uint8_t MPU6050_LastWhoAmI = 0xFF;
HAL_StatusTypeDef MPU6050_LastStatus = HAL_ERROR;

/* ---------------- DUSUK SEVIYE I2C YARDIMCILARI ---------------- */

static HAL_StatusTypeDef MPU6050_WriteReg(uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(mpu_i2c, MPU6050_I2C_ADDR, reg,
                              I2C_MEMADD_SIZE_8BIT, &value, 1, I2C_TIMEOUT);
}

static HAL_StatusTypeDef MPU6050_ReadRegs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    /* Not: Gercek donanimda tek seferlik (burst) okuma sorunsuz calisir.
     * PICSimLab gibi bazi I2C emulatorlerinde uzun burst okumalar sorun
     * cikarabiliyor; oyle bir durumla karsilasirsaniz, BMP280 surucusunde
     * yapildigi gibi bu fonksiyonu byte-byte okuyacak sekilde
     * degistirebilirsiniz (register'i tek tek artirarak). */
    return HAL_I2C_Mem_Read(mpu_i2c, MPU6050_I2C_ADDR, reg,
                             I2C_MEMADD_SIZE_8BIT, buf, len, I2C_TIMEOUT);
}

/* ---------------- OLCEKLENDIRME KATSAYILARINI SEC ---------------- */

static void MPU6050_SetAccelScale(MPU6050_AccelRange_t range)
{
    switch (range)
    {
        case MPU6050_ACCEL_RANGE_2G:  accel_lsb_per_g = 16384.0f; break;
        case MPU6050_ACCEL_RANGE_4G:  accel_lsb_per_g = 8192.0f;  break;
        case MPU6050_ACCEL_RANGE_8G:  accel_lsb_per_g = 4096.0f;  break;
        case MPU6050_ACCEL_RANGE_16G: accel_lsb_per_g = 2048.0f;  break;
        default: accel_lsb_per_g = 16384.0f; break;
    }
}

static void MPU6050_SetGyroScale(MPU6050_GyroRange_t range)
{
    switch (range)
    {
        case MPU6050_GYRO_RANGE_250DPS:  gyro_lsb_per_dps = 131.0f; break;
        case MPU6050_GYRO_RANGE_500DPS:  gyro_lsb_per_dps = 65.5f;  break;
        case MPU6050_GYRO_RANGE_1000DPS: gyro_lsb_per_dps = 32.8f;  break;
        case MPU6050_GYRO_RANGE_2000DPS: gyro_lsb_per_dps = 16.4f;  break;
        default: gyro_lsb_per_dps = 131.0f; break;
    }
}

/* ---------------- PUBLIC API ---------------- */

HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c,
                                MPU6050_AccelRange_t accelRange,
                                MPU6050_GyroRange_t gyroRange)
{
    mpu_i2c = hi2c;
    uint8_t who = 0;

    /* 1) Cihaz gercekten hatta mi? */
    MPU6050_LastStatus = MPU6050_ReadRegs(MPU6050_REG_WHO_AM_I, &who, 1);
    MPU6050_LastWhoAmI = who;

    if (MPU6050_LastStatus != HAL_OK)
        return HAL_ERROR;

    if (who != MPU6050_WHO_AM_I_VAL)
        return HAL_ERROR; /* Yanlis adres / yanlis kablolama / cihaz yok */

    /* 2) Uyku modundan cikar (PWR_MGMT_1 reg, sifirlanmis reset degeri 0x40 -> sleep=1)
     *    0x00 yazarak: sleep=0, clock source = internal 8MHz oscillator */
    if (MPU6050_WriteReg(MPU6050_REG_PWR_MGMT_1, 0x00) != HAL_OK)
        return HAL_ERROR;
    HAL_Delay(10);

    /* 3) Ornekleme hizi bolucusu: Sample Rate = Gyro Output Rate / (1 + SMPLRT_DIV)
     *    DLPF etkinken Gyro Output Rate = 1kHz -> SMPLRT_DIV=7 => 125 Hz ornekleme */
    if (MPU6050_WriteReg(MPU6050_REG_SMPLRT_DIV, 0x07) != HAL_OK)
        return HAL_ERROR;

    /* 4) Dijital alcak geciren filtre (DLPF): ~44Hz bant genisligi (orta seviye, titresim gurultusunu azaltir) */
    if (MPU6050_WriteReg(MPU6050_REG_CONFIG, 0x03) != HAL_OK)
        return HAL_ERROR;

    /* 5) Jiroskop olcek araligi */
    if (MPU6050_WriteReg(MPU6050_REG_GYRO_CONFIG, (uint8_t)gyroRange) != HAL_OK)
        return HAL_ERROR;
    MPU6050_SetGyroScale(gyroRange);

    /* 6) Ivmeolcer olcek araligi */
    if (MPU6050_WriteReg(MPU6050_REG_ACCEL_CONFIG, (uint8_t)accelRange) != HAL_OK)
        return HAL_ERROR;
    MPU6050_SetAccelScale(accelRange);

    HAL_Delay(10);
    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_ReadAll(MPU6050_Data_t *out)
{
    uint8_t buf[14]; /* ACCEL(6) + TEMP(2) + GYRO(6), 0x3B..0x48 ardisik */

    if (MPU6050_ReadRegs(MPU6050_REG_ACCEL_XOUT_H, buf, 14) != HAL_OK)
        return HAL_ERROR;

    int16_t raw_ax = (int16_t)((buf[0] << 8)  | buf[1]);
    int16_t raw_ay = (int16_t)((buf[2] << 8)  | buf[3]);
    int16_t raw_az = (int16_t)((buf[4] << 8)  | buf[5]);
    int16_t raw_t  = (int16_t)((buf[6] << 8)  | buf[7]);
    int16_t raw_gx = (int16_t)((buf[8] << 8)  | buf[9]);
    int16_t raw_gy = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t raw_gz = (int16_t)((buf[12] << 8) | buf[13]);

    out->accel_x_g = raw_ax / accel_lsb_per_g;
    out->accel_y_g = raw_ay / accel_lsb_per_g;
    out->accel_z_g = raw_az / accel_lsb_per_g;

    out->gyro_x_dps = raw_gx / gyro_lsb_per_dps;
    out->gyro_y_dps = raw_gy / gyro_lsb_per_dps;
    out->gyro_z_dps = raw_gz / gyro_lsb_per_dps;

    /* InvenSense datasheet formulu: Temp(C) = (raw / 340) + 36.53 */
    out->temperature_C = ((float)raw_t / 340.0f) + 36.53f;

    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_ReadAccel(MPU6050_Data_t *out)
{
    uint8_t buf[6];

    if (MPU6050_ReadRegs(MPU6050_REG_ACCEL_XOUT_H, buf, 6) != HAL_OK)
        return HAL_ERROR;

    int16_t raw_ax = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_ay = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_az = (int16_t)((buf[4] << 8) | buf[5]);

    out->accel_x_g = raw_ax / accel_lsb_per_g;
    out->accel_y_g = raw_ay / accel_lsb_per_g;
    out->accel_z_g = raw_az / accel_lsb_per_g;

    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_ReadGyro(MPU6050_Data_t *out)
{
    uint8_t buf[6];

    if (MPU6050_ReadRegs(MPU6050_REG_GYRO_XOUT_H, buf, 6) != HAL_OK)
        return HAL_ERROR;

    int16_t raw_gx = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_gy = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_gz = (int16_t)((buf[4] << 8) | buf[5]);

    out->gyro_x_dps = raw_gx / gyro_lsb_per_dps;
    out->gyro_y_dps = raw_gy / gyro_lsb_per_dps;
    out->gyro_z_dps = raw_gz / gyro_lsb_per_dps;

    return HAL_OK;
}
