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
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "stdlib.h"
#include "uart.h"
#include "adc_x.h"
#include "MLX90614.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FACTOR_DIVISOR   5.0f   // (R1+R2)/R2, debe ser >= 4.55 para 15 V
#define FORMATO_TELEPLOT 1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
char    buffer[96];
float   voltaje;     // Potenciómetro en PA0
float   vin15;       // Divisor de voltaje 0-15 V en PA1
float   temp_obj;    // Temperatura del objeto (MLX90614), °C
float   temp_amb;    // Temperatura ambiente (MLX90614), °C
uint8_t mlx_ok;      // Flag de inicialización del MLX90614
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void UART_WriteText(UART_HandleTypeDef *huart, const char *txt)
{
  HAL_UART_Transmit(huart, (uint8_t *)txt, strlen(txt), 100);
}

static uint16_t ADC_ReadAvg(uint32_t channel)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel      = channel;
  sConfig.Rank         = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  uint32_t sum = 0;
  for (int i = 0; i < 16; i++)
  {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    sum += HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
  }
  return (uint16_t)(sum / 16);
}
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
UART_WriteText(&huart1, "Iniciando laboratorio de Sistemas Embebidos\r\n");

mlx_ok = MLX90614_Init();
if (mlx_ok == 0)
  UART_WriteText(&huart1, "MLX90614 no inicializado correctamente\r\n");
else
  UART_WriteText(&huart1, "MLX90614 inicializado correctamente\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // -- ADC: PA0 = potenciómetro, PA1 = divisor 0-15 V
    voltaje = ADC_ReadAvg(ADC_CHANNEL_0) * 3.3f / 4095.0f;
    vin15   = ADC_ReadAvg(ADC_CHANNEL_1) * 3.3f / 4095.0f * FACTOR_DIVISOR;

    // -- MLX90614 por I2C (en °C)
    temp_obj = (float)MLX90614_readObjectTempC();
    temp_amb = (float)MLX90614_readAmbientTempC();
    if (isnan(temp_obj)) temp_obj = -999.0f;   // -999 indica error de lectura
    if (isnan(temp_amb)) temp_amb = -999.0f;

    // -- Envío por USART1 (115200 baud)
#if FORMATO_TELEPLOT
    sprintf(buffer, ">Potenciometro:%.2f\r\n", voltaje);
    UART_WriteText(&huart1, buffer);
    sprintf(buffer, ">Voltaje15V:%.2f\r\n", vin15);
    UART_WriteText(&huart1, buffer);
    sprintf(buffer, ">TempObjeto:%.2f\r\n", temp_obj);
    UART_WriteText(&huart1, buffer);
    sprintf(buffer, ">TempAmbiente:%.2f\r\n", temp_amb);
    UART_WriteText(&huart1, buffer);
#else
    sprintf(buffer, "POT:%.2f,VIN:%.2f,TOBJ:%.2f,TAMB:%.2f\r\n",
            voltaje, vin15, temp_obj, temp_amb);
    UART_WriteText(&huart1, buffer);
#endif

    HAL_Delay(200);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
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
