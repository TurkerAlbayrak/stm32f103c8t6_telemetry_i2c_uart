/*
 * bmp280.c
 *
 * STM32 HAL tabanli BMP280 (I2C) surucusu
 * Kompanzasyon formulleri Bosch resmi datasheet'inden alinmistir (double versiyon).
 */

#include "bmp280.h"
#include <math.h>

static I2C_HandleTypeDef *bmp_i2c;
static BMP280_CalibData_t calib;
static double t_fine;
static float seaLevelPressure_hPa = 1013.25f;

uint8_t BMP280_LastChipID = 0xFF;
HAL_StatusTypeDef BMP280_LastStatus = HAL_ERROR;
HAL_StatusTypeDef BMP280_CalibStatus = HAL_ERROR;

#define I2C_TIMEOUT 100

/* ---------------- DUSUK SEVIYE I2C YARDIMCILARI ---------------- */

static HAL_StatusTypeDef BMP280_WriteReg(uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(bmp_i2c, BMP280_I2C_ADDR, reg,
                              I2C_MEMADD_SIZE_8BIT, &value, 1, I2C_TIMEOUT);
}

static HAL_StatusTypeDef BMP280_ReadRegs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(bmp_i2c, BMP280_I2C_ADDR, reg,
                             I2C_MEMADD_SIZE_8BIT, buf, len, I2C_TIMEOUT);
}

/* ---------------- KALIBRASYON VERISINI OKUMA ---------------- */

static HAL_StatusTypeDef BMP280_ReadCalibration(void)
{
    uint8_t buf[24];
    HAL_StatusTypeDef status;

    /* Tek seferde 24 byte okumak yerine, byte byte okuyoruz.
     * Bazi I2C emulatorleri (PICSimLab/qemu-stm32 dahil) uzun burst
     * okumalarda sorun cikarabiliyor; tek byte okuma cok daha guvenilir. */
    for (int i = 0; i < 24; i++)
    {
        status = BMP280_ReadRegs(BMP280_REG_DIG_T1 + i, &buf[i], 1);
        if (status != HAL_OK)
        {
            BMP280_CalibStatus = status;
            return status;
        }
    }
    BMP280_CalibStatus = HAL_OK;

    calib.dig_T1 = (uint16_t)(buf[0]  | (buf[1]  << 8));
    calib.dig_T2 = (int16_t) (buf[2]  | (buf[3]  << 8));
    calib.dig_T3 = (int16_t) (buf[4]  | (buf[5]  << 8));

    calib.dig_P1 = (uint16_t)(buf[6]  | (buf[7]  << 8));
    calib.dig_P2 = (int16_t) (buf[8]  | (buf[9]  << 8));
    calib.dig_P3 = (int16_t) (buf[10] | (buf[11] << 8));
    calib.dig_P4 = (int16_t) (buf[12] | (buf[13] << 8));
    calib.dig_P5 = (int16_t) (buf[14] | (buf[15] << 8));
    calib.dig_P6 = (int16_t) (buf[16] | (buf[17] << 8));
    calib.dig_P7 = (int16_t) (buf[18] | (buf[19] << 8));
    calib.dig_P8 = (int16_t) (buf[20] | (buf[21] << 8));
    calib.dig_P9 = (int16_t) (buf[22] | (buf[23] << 8));

    return HAL_OK;
}

/* ---------------- BOSCH KOMPANZASYON FORMULLERI ---------------- */

static double BMP280_CompensateT(int32_t adc_T)
{
    double var1, var2, T;
    var1 = (((double)adc_T) / 16384.0 - ((double)calib.dig_T1) / 1024.0) * ((double)calib.dig_T2);
    var2 = ((((double)adc_T) / 131072.0 - ((double)calib.dig_T1) / 8192.0) *
            (((double)adc_T) / 131072.0 - ((double)calib.dig_T1) / 8192.0)) * ((double)calib.dig_T3);
    t_fine = var1 + var2;
    T = (var1 + var2) / 5120.0;
    return T;
}

static double BMP280_CompensateP(int32_t adc_P)
{
    double var1, var2, p;
    var1 = ((double)t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((double)calib.dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)calib.dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (((double)calib.dig_P4) * 65536.0);
    var1 = (((double)calib.dig_P3) * var1 * var1 / 524288.0 + ((double)calib.dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)calib.dig_P1);

    if (var1 == 0.0) return 0; /* sifira bolme koruma */

    p = 1048576.0 - (double)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)calib.dig_P9) * p * p / 2147483648.0;
    var2 = p * ((double)calib.dig_P8) / 32768.0;
    p = p + (var1 + var2 + ((double)calib.dig_P7)) / 16.0;

    return p; /* Pa */
}

/* ---------------- PUBLIC API ---------------- */

HAL_StatusTypeDef BMP280_Init(I2C_HandleTypeDef *hi2c)
{
    bmp_i2c = hi2c;
    uint8_t chip_id = 0;

    /* 1) Cihaz gercekten hatta mi? */
    BMP280_LastStatus = BMP280_ReadRegs(BMP280_REG_CHIP_ID, &chip_id, 1);
    BMP280_LastChipID = chip_id;

    if (BMP280_LastStatus != HAL_OK)
        return HAL_ERROR;

    if (chip_id != BMP280_CHIP_ID_VAL)
        return HAL_ERROR; /* Yanlis adres / yanlis kablolama / cihap yok */

    /* 2) Yazilimsal reset (opsiyonel ama guvenli) */
    BMP280_WriteReg(BMP280_REG_RESET, BMP280_SOFT_RESET_VAL);
    HAL_Delay(10);

    /* 3) Kalibrasyon verisini oku */
    if (BMP280_ReadCalibration() != HAL_OK)
        return HAL_ERROR;

    /* 4) config: standby 0.5ms, filtre kapali (0b000_000_0 = 0x00) */
    BMP280_WriteReg(BMP280_REG_CONFIG, 0x00);

    /* 5) ctrl_meas: osrs_t=x2(010), osrs_p=x16(101), mode=normal(11)
     *    -> 0b010_101_11 = 0x57  (yuksek cozunurluklu basinc olcumu icin) */
    BMP280_WriteReg(BMP280_REG_CTRL_MEAS, 0x57);

    HAL_Delay(10);
    return HAL_OK;
}

void BMP280_SetSeaLevelPressure(float seaLevel_hPa)
{
    seaLevelPressure_hPa = seaLevel_hPa;
}

HAL_StatusTypeDef BMP280_ReadAll(BMP280_Data_t *out)
{
    uint8_t buf[6];

    /* PRESS_MSB..TEMP_XLSB araligini (0xF7..0xFC, 6 byte) tek tek oku.
     * Burst okuma yerine byte byte okumak emulator uyumlulugu icin daha guvenli. */
    for (int i = 0; i < 6; i++)
    {
        if (BMP280_ReadRegs(BMP280_REG_PRESS_MSB + i, &buf[i], 1) != HAL_OK)
            return HAL_ERROR;
    }

    int32_t adc_P = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | (buf[2] >> 4);
    int32_t adc_T = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | (buf[5] >> 4);

    double T = BMP280_CompensateT(adc_T);   /* t_fine burada guncellenir, once cagrilmali */
    double P = BMP280_CompensateP(adc_P);   /* Pa */

    out->temperature_C = (float)T;
    out->pressure_Pa   = (float)P;
    out->pressure_hPa  = (float)(P / 100.0);

    /* Barometrik yukseklik formulu */
    out->altitude_m = (float)(44330.0 * (1.0 - pow(out->pressure_hPa / seaLevelPressure_hPa, 0.1903)));

    return HAL_OK;
}
