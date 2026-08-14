/*
 * mpu6050.h
 *
 * STM32 HAL tabanli MPU6050 (ivmeolcer + jiroskop, I2C) surucusu
 * Gercek donanimda (Blue Pill + MPU6050 breakout) kullanima uygundur.
 *
 * Kullanim:
 *   1) CubeMX'te I2C1'i etkinlestir (PB6=SCL, PB7=SDA)
 *   2) main.c'de "#include "mpu6050.h"" ekle
 *   3) MPU6050_Init(&hi2c1) cagir
 *   4) Donguide MPU6050_ReadAll(&data) ile oku
 */

#ifndef INC_MPU6050_H_
#define INC_MPU6050_H_

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* ---------------- I2C ADRESI ----------------
 * MPU6050'nin AD0 pini GND'ye cekiliyse   -> adres 0x68 (cogu breakout kartinda varsayilan)
 * MPU6050'nin AD0 pini VCC'ye cekiliyse   -> adres 0x69
 */
#define MPU6050_I2C_ADDR      (0x68 << 1)   /* AD0 -> GND varsayimi (HAL 8-bit adres formatinda) */
/* #define MPU6050_I2C_ADDR   (0x69 << 1) */ /* AD0 -> VCC ise bunu kullanin */

/* ---------------- REGISTER HARITASI ---------------- */
#define MPU6050_REG_SMPLRT_DIV     0x19
#define MPU6050_REG_CONFIG         0x1A
#define MPU6050_REG_GYRO_CONFIG    0x1B
#define MPU6050_REG_ACCEL_CONFIG   0x1C
#define MPU6050_REG_ACCEL_XOUT_H   0x3B
#define MPU6050_REG_TEMP_OUT_H     0x41
#define MPU6050_REG_GYRO_XOUT_H    0x43
#define MPU6050_REG_PWR_MGMT_1     0x6B
#define MPU6050_REG_WHO_AM_I       0x75

#define MPU6050_WHO_AM_I_VAL       0x68

/* Olceklendirme secenekleri (GYRO_CONFIG / ACCEL_CONFIG FS_SEL bitleri) */
typedef enum {
    MPU6050_ACCEL_RANGE_2G  = 0x00,  /* +-2g,  hassasiyet: 16384 LSB/g   */
    MPU6050_ACCEL_RANGE_4G  = 0x08,  /* +-4g,  hassasiyet: 8192  LSB/g   */
    MPU6050_ACCEL_RANGE_8G  = 0x10,  /* +-8g,  hassasiyet: 4096  LSB/g   */
    MPU6050_ACCEL_RANGE_16G = 0x18   /* +-16g, hassasiyet: 2048  LSB/g   */
} MPU6050_AccelRange_t;

typedef enum {
    MPU6050_GYRO_RANGE_250DPS  = 0x00, /* +-250 dps,  hassasiyet: 131.0 LSB/(deg/s) */
    MPU6050_GYRO_RANGE_500DPS  = 0x08, /* +-500 dps,  hassasiyet: 65.5  LSB/(deg/s) */
    MPU6050_GYRO_RANGE_1000DPS = 0x10, /* +-1000 dps, hassasiyet: 32.8  LSB/(deg/s) */
    MPU6050_GYRO_RANGE_2000DPS = 0x18  /* +-2000 dps, hassasiyet: 16.4  LSB/(deg/s) */
} MPU6050_GyroRange_t;

/* ---------------- VERI YAPISI ---------------- */
typedef struct {
    float accel_x_g, accel_y_g, accel_z_g;   /* ivme, g cinsinden       */
    float gyro_x_dps, gyro_y_dps, gyro_z_dps; /* acisal hiz, derece/sn  */
    float temperature_C;                      /* dahili sicaklik sensoru */
} MPU6050_Data_t;

/* ---------------- FONKSIYONLAR ---------------- */

/**
 * @brief MPU6050'yi baslatir: who-am-i dogrulamasi, uyku modundan cikarma,
 *        ornekleme hizi ve olcek araligi ayarlari.
 * @param hi2c        Kullanilacak I2C handle (ornek: &hi2c1)
 * @param accelRange  Ivmeolcer olcek araligi
 * @param gyroRange   Jiroskop olcek araligi
 * @retval HAL_OK basarili, HAL_ERROR who-am-i uyusmuyorsa / iletisim hatasi
 */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c,
                                MPU6050_AccelRange_t accelRange,
                                MPU6050_GyroRange_t gyroRange);

/**
 * @brief Ivme, jiroskop ve sicaklik verilerini okuyup olceklendirilmis
 *        (g, derece/sn, C) degerlere cevirir.
 */
HAL_StatusTypeDef MPU6050_ReadAll(MPU6050_Data_t *out);

/**
 * @brief Sadece ivme verisini okumak icin (daha hizli dongu istenirse)
 */
HAL_StatusTypeDef MPU6050_ReadAccel(MPU6050_Data_t *out);

/**
 * @brief Sadece jiroskop verisini okumak icin
 */
HAL_StatusTypeDef MPU6050_ReadGyro(MPU6050_Data_t *out);

/* ---------------- TESHIS (DEBUG) ---------------- */
extern uint8_t MPU6050_LastWhoAmI;
extern HAL_StatusTypeDef MPU6050_LastStatus;

#endif /* INC_MPU6050_H_ */
