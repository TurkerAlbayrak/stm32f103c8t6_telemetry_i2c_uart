/* ============================================================
 * TUM SISTEM: BMP280 + MPU6050 + Telemetri
 * Bu dosyanin ICERIGINI direkt derlemeye eklemeyin. CubeIDE'nin
 * urettigi main.c icindeki ayni isimli
 * "/* USER CODE BEGIN ... * /" - "/* USER CODE END ... * /"
 * bloklarinin ARASINA, asagidaki sirayla yapistirin.
 * ============================================================ */


/* USER CODE BEGIN Includes */
#include "bmp280.h"
#include "mpu6050.h"
#include "telemetry.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */


/* USER CODE BEGIN PV */
BMP280_Data_t       bmp280_data;
MPU6050_Data_t       mpu6050_data;
TelemetryPayload_t   tlm_payload;
/* USER CODE END PV */


/* USER CODE BEGIN PFP */
#ifdef __GNUC__
int __io_putchar(int ch);
#endif
/* USER CODE END PFP */


/* USER CODE BEGIN 0 */
#ifdef __GNUC__
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
#endif
/* USER CODE END 0 */


/* main() icinde MX_I2C1_Init() ve MX_USART1_UART_Init() cagrilarindan
 * SONRA, while(1) dongusunden ONCE: */

/* USER CODE BEGIN 2 */
printf("\r\n========================================\r\n");
printf(" Mini Uydu OBC - Baslatiliyor\r\n");
printf("========================================\r\n");

/* --- BMP280 --- */
HAL_StatusTypeDef bmpInit = BMP280_Init(&hi2c1);
printf("[BMP280] ChipID-status=%d ChipID=0x%02X (beklenen 0x58) -> %s\r\n",
       BMP280_LastStatus, BMP280_LastChipID,
       (bmpInit == HAL_OK) ? "OK" : "HATA");
if (bmpInit != HAL_OK)
{
    printf("HATA: BMP280 baslatilamadi!\r\n");
    Error_Handler();
}

/* --- MPU6050 --- */
HAL_StatusTypeDef mpuInit = MPU6050_Init(&hi2c1, MPU6050_ACCEL_RANGE_2G, MPU6050_GYRO_RANGE_250DPS);
printf("[MPU6050] WhoAmI-status=%d WhoAmI=0x%02X (beklenen 0x68) -> %s\r\n",
       MPU6050_LastStatus, MPU6050_LastWhoAmI,
       (mpuInit == HAL_OK) ? "OK" : "HATA");
if (mpuInit != HAL_OK)
{
    printf("HATA: MPU6050 baslatilamadi!\r\n");
    Error_Handler();
}

/* --- Telemetri --- */
Telemetry_Init(&huart1); /* Ayni UART: hem okunabilir metin hem ikili cerceve */
printf("[TLM] Telemetri modulu hazir (USART1, 115200 baud).\r\n");
printf("========================================\r\n");
printf("Tum sistemler hazir, ana donguye giriliyor...\r\n\r\n");
/* USER CODE END 2 */


/* USER CODE BEGIN WHILE */
while (1)
{
    HAL_StatusTypeDef bmpStatus = BMP280_ReadAll(&bmp280_data);
    HAL_StatusTypeDef mpuStatus = MPU6050_ReadAll(&mpu6050_data);

    if (bmpStatus == HAL_OK && mpuStatus == HAL_OK)
    {
        /* Telemetri paketini doldur */
        tlm_payload.temperature_C = bmp280_data.temperature_C;
        tlm_payload.pressure_hPa  = bmp280_data.pressure_hPa;
        tlm_payload.altitude_m    = bmp280_data.altitude_m;

        tlm_payload.accel_x_g = mpu6050_data.accel_x_g;
        tlm_payload.accel_y_g = mpu6050_data.accel_y_g;
        tlm_payload.accel_z_g = mpu6050_data.accel_z_g;

        tlm_payload.gyro_x_dps = mpu6050_data.gyro_x_dps;
        tlm_payload.gyro_y_dps = mpu6050_data.gyro_y_dps;
        tlm_payload.gyro_z_dps = mpu6050_data.gyro_z_dps;

        tlm_payload.imu_temp_C   = mpu6050_data.temperature_C;
        tlm_payload.timestamp_ms = HAL_GetTick();

        /* Ikili telemetri cercevesini gonder */
        Telemetry_Send(&tlm_payload);

        /* Insan-okunabilir ozet */
        printf("[TLM #%lu] T=%.1fC P=%.1fhPa Alt=%.1fm | Ax=%.2f Ay=%.2f Az=%.2f g | Gx=%.1f Gy=%.1f Gz=%.1f dps\r\n",
               (unsigned long)Telemetry_GetSentCount(),
               tlm_payload.temperature_C, tlm_payload.pressure_hPa, tlm_payload.altitude_m,
               tlm_payload.accel_x_g, tlm_payload.accel_y_g, tlm_payload.accel_z_g,
               tlm_payload.gyro_x_dps, tlm_payload.gyro_y_dps, tlm_payload.gyro_z_dps);
    }
    else
    {
        printf("HATA: Sensor okuma basarisiz (BMP=%d, MPU=%d) - paket atlandi.\r\n", bmpStatus, mpuStatus);
    }

    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(500);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
}
/* USER CODE END 3 */
