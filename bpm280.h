/*
 * bmp280.h
 *
 * STM32 HAL tabanli BMP280 (I2C) surucusu
 * Blue Pill (STM32F103C8T6) + PICSimLab simulasyonu icin hazirlanmistir.
 *
 * Kullanim:
 *   1) CubeMX'te I2C1'i etkinlestir (PB6=SCL, PB7=SDA)
 *   2) main.c'de "#include "bmp280.h"" ekle
 *   3) BMP280_Init(&hi2c1) cagir
 *   4) Donguide BMP280_ReadAll(&data) ile oku
 */

#ifndef INC_BMP280_H_
#define INC_BMP280_H_

#include "stm32f1xx_hal.h"   /* Kullandiginiz STM32 ailesine gore CubeIDE bu dosyayi otomatik dogru isimle uretir */
#include <stdint.h>

/* ---------------- I2C ADRESI ----------------
 * BMP280'in SDO pini GND'ye cekiliyse   -> adres 0x76
 * BMP280'in SDO pini VCC'ye cekili/NC ise -> adres 0x77
 * PICSimLab'te BMP280 parcasini eklerken SDO pinini nereye bagladiysaniz
 * asagidaki tanimi ona gore secin (ikisinden sadece biri define edilmeli).
 */
#define BMP280_I2C_ADDR      (0x76 << 1)   /* SDO -> GND varsayimi (HAL 8-bit adres formatinda) */
/* #define BMP280_I2C_ADDR   (0x77 << 1) */ /* SDO -> VCC / NC ise bunu kullanin */

/* ---------------- REGISTER HARITASI ---------------- */
#define BMP280_REG_DIG_T1     0x88
#define BMP280_REG_CHIP_ID    0xD0
#define BMP280_REG_RESET      0xE0
#define BMP280_REG_STATUS     0xF3
#define BMP280_REG_CTRL_MEAS  0xF4
#define BMP280_REG_CONFIG     0xF5
#define BMP280_REG_PRESS_MSB  0xF7
#define BMP280_REG_TEMP_MSB   0xFA

#define BMP280_CHIP_ID_VAL    0x58
#define BMP280_SOFT_RESET_VAL 0xB6

/* ---------------- VERI YAPILARI ---------------- */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} BMP280_CalibData_t;

typedef struct {
    float temperature_C;   /* Santigrat derece */
    float pressure_Pa;     /* Pascal */
    float pressure_hPa;    /* hektoPascal / milibar */
    float altitude_m;      /* Deniz seviyesine gore yaklasik yukseklik (m) */
} BMP280_Data_t;

/* ---------------- FONKSIYONLAR ---------------- */

/**
 * @brief BMP280'i baslatir: chip-id dogrulamasi, kalibrasyon verisini okur,
 *        normal mode + oversampling ayarlarini yapar.
 * @retval HAL_OK basarili, HAL_ERROR chip-id uyusmuyorsa / iletisim hatasi
 */
HAL_StatusTypeDef BMP280_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Sicaklik ve basinci okuyup kompanzasyon hesaplarini yapar.
 */
HAL_StatusTypeDef BMP280_ReadAll(BMP280_Data_t *out);

/**
 * @brief Deniz seviyesi referans basincini (hPa) degistirmek icin (varsayilan 1013.25)
 */
void BMP280_SetSeaLevelPressure(float seaLevel_hPa);

/* ---------------- TESHIS (DEBUG) ---------------- */
/* BMP280_Init icinde okunan son chip-id degeri ve HAL donus kodu.
 * Beklenen: BMP280_LastChipID == 0x58, BMP280_LastStatus == HAL_OK (0) */
extern uint8_t BMP280_LastChipID;
extern HAL_StatusTypeDef BMP280_LastStatus;
extern HAL_StatusTypeDef BMP280_CalibStatus;

#endif /* INC_BMP280_H_ */
