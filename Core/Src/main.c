/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "bmp280.h"
#include "mpu6050.h"
#include "telemetry.h"
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
BMP280_Data_t       bmp280_data;
MPU6050_Data_t       mpu6050_data;
TelemetryPayload_t   tlm_payload;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
#ifdef __GNUC__
int __io_putchar(int ch);
#endif
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#ifdef __GNUC__
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
#endif
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
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

  /* Infinite loop */
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

	          /* Hizli gorsel dogrulama: son cercevenin hex dokumu.
	           * com0com/tty0tty + Python ile test etmeye basladiktan sonra
	           * bu blogu yorum satiri yapabilirsiniz. */
	          const uint8_t *frameBuf; uint8_t frameLen;
	          Telemetry_GetLastFrame(&frameBuf, &frameLen);
	          printf("[HEX] ");
	          for (uint8_t i = 0; i < frameLen; i++) printf("%02X ", frameBuf[i]);
	          printf("\r\n");

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
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
