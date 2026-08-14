/*
 * telemetry.h
 *
 * Basit, senkron-baytli + checksum'lu ikili telemetri cercevesi.
 * UART, HC-12, LoRa (SX1278) gibi farkli fiziksel tasiyicilar uzerinden
 * AYNI paket formati kullanilabilir - sadece "gonderme" fonksiyonunun
 * icini (HAL_UART_Transmit yerine LoRa_Send vb.) degistirmeniz yeterli.
 *
 * Kullanim:
 *   1) Telemetry_Init(&huart1) cagir (bir kere, baslangicta)
 *   2) Her donguide TelemetryPayload_t doldur, Telemetry_Send() cagir
 */

#ifndef INC_TELEMETRY_H_
#define INC_TELEMETRY_H_

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define TELEMETRY_SYNC_BYTE_1   0xAA
#define TELEMETRY_SYNC_BYTE_2   0x55

/* ---------------- TELEMETRI YUKU (PAYLOAD) ----------------
 * __attribute__((packed)) ile derleyicinin araya hizalama (padding)
 * byte'i eklemesi engellenir - boylece STM32'de urettigimiz byte dizisi
 * ile PC/Python tarafinda struct.unpack ile cozdugumuz dizinin boyutu
 * birebir ayni kalir. */
typedef struct __attribute__((packed))
{
    float    temperature_C;    /* BMP280 sicaklik */
    float    pressure_hPa;     /* BMP280 basinc */
    float    altitude_m;       /* BMP280 irtifa */

    float    accel_x_g;        /* MPU6050 ivme */
    float    accel_y_g;
    float    accel_z_g;

    float    gyro_x_dps;       /* MPU6050 acisal hiz */
    float    gyro_y_dps;
    float    gyro_z_dps;

    float    imu_temp_C;       /* MPU6050 dahili sicaklik */

    uint32_t timestamp_ms;     /* HAL_GetTick() - kalkistan/reset'ten bu yana gecen ms */
} TelemetryPayload_t;

/* ---------------- FONKSIYONLAR ---------------- */

/**
 * @brief Telemetri modulunu baslatir (UART handle'ini saklar).
 */
void Telemetry_Init(UART_HandleTypeDef *huart);

/**
 * @brief Verilen payload'i cerceveleyip (sync+id+len+payload+checksum)
 *        UART uzerinden gonderir. Her cagrida packet_id otomatik artar.
 * @retval HAL_OK basarili gonderim, HAL_ERROR/TIMEOUT UART hatasi
 */
HAL_StatusTypeDef Telemetry_Send(const TelemetryPayload_t *payload);

/**
 * @brief Su ana kadar gonderilen paket sayisini dondurur (debug/istatistik icin).
 */
uint32_t Telemetry_GetSentCount(void);

/**
 * @brief Hizli gorsel dogrulama (Python/com0com kurmadan once) icin:
 *        en son gonderilen cercevenin ham baytlarini ve boyutunu dondurur.
 *        main.c'de bunu hex olarak printf ile yazdirip elle SYNC(0xAA 0x55)
 *        ve checksum'i goz kontroluyle dogrulayabilirsiniz.
 */
void Telemetry_GetLastFrame(const uint8_t **outBuf, uint8_t *outLen);

#endif /* INC_TELEMETRY_H_ */
