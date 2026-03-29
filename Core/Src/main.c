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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_x-cube-ai.h"
#include <string.h>
#include <stdio.h>

#define INPUT_SIZE_BYTES        20800
#define INPUT_SIZE_FLOATS       5200
#define OUTPUT_CLASSES          29

#if defined(__ICCARM__)
  #define AI_ALIGNED(x) _Pragma("data_alignment=" #x)
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
  #define AI_ALIGNED(x) __attribute__((aligned(x)))
#elif defined(__GNUC__)
  #define AI_ALIGNED(x) __attribute__((aligned(x)))
#endif

AI_ALIGNED(32) static float ai_input[INPUT_SIZE_FLOATS];
AI_ALIGNED(32) static float ai_output[OUTPUT_CLASSES];

static char tx_buffer[64];
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define STATE_IDLE          0
#define STATE_WAITING_DATA  1
#define STATE_PROCESSING    2
#define CPU_FREQUENCY_HZ   168000000ULL
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint8_t rx_byte;
static uint8_t rx_buffer[INPUT_SIZE_BYTES];
static volatile uint32_t rx_index = 0;
static volatile uint8_t protocol_state = STATE_IDLE;
static volatile uint8_t data_ready = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void MX_CRC_Init(void)
{
  __HAL_RCC_CRC_CLK_ENABLE();
}

static void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static uint32_t DWT_GetCycles(void)
{
    return DWT->CYCCNT;
}

static uint32_t CyclesToUs(uint32_t cycles)
{
    return cycles / (CPU_FREQUENCY_HZ / 1000000);
}

static void EnableCaches(void)
{
    if (HAL_GetREVID() != 0x1001)
    {
         __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    }
    __HAL_FLASH_DATA_CACHE_ENABLE();
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
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

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  EnableCaches();
  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  MX_CRC_Init();
  DWT_Init();
  int ai_status = MX_X_CUBE_AI_Init();
  ai_run(ai_input, ai_output);
  sprintf(tx_buffer, "AI READY (Warmup Done). Init: %d\r\n", ai_status);
	HAL_UART_Transmit(&huart1, (uint8_t*)tx_buffer, strlen(tx_buffer), HAL_MAX_DELAY);

	protocol_state = STATE_IDLE;
	rx_index = 0;
	data_ready = 0;
	HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	  {
		  if (data_ready) {
			  data_ready = 0;
			  memcpy(ai_input, rx_buffer, INPUT_SIZE_BYTES); // Copy data

			  HAL_SuspendTick();

			  uint32_t start_cycles = DWT_GetCycles();
			  int result = ai_run(ai_input, ai_output);
			  uint32_t end_cycles = DWT_GetCycles();

			  HAL_ResumeTick();

			  if (result != 0) {
				  sprintf(tx_buffer, "ERR:INFERENCE:%d\n", result);
				  HAL_UART_Transmit(&huart1, (uint8_t*)tx_buffer, strlen(tx_buffer), HAL_MAX_DELAY);
			  } else {
				  int max_class = 0;
				  float max_val = ai_output[0];
				  for (int i = 1; i < OUTPUT_CLASSES; i++) {
					  if (ai_output[i] > max_val) {
						  max_val = ai_output[i];
						  max_class = i;
					  }
				  }
				  int conf_int = (int)(max_val * 1000);

				  uint32_t total_cycles;
				  if (end_cycles >= start_cycles) {
					  total_cycles = end_cycles - start_cycles;
				  } else {
					  total_cycles = (0xFFFFFFFF - start_cycles) + end_cycles + 1;
				  }

				  uint32_t time_us = CyclesToUs(total_cycles);

				  sprintf(tx_buffer, "RESULT:%d:%d:%lu\n", max_class, conf_int, time_us);
				  HAL_UART_Transmit(&huart1, (uint8_t*)tx_buffer, strlen(tx_buffer), HAL_MAX_DELAY);
			  }

			  protocol_state = STATE_IDLE;
			  rx_index = 0;
			  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
		  }
	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
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
  huart1.Init.BaudRate = 921600;
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
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (protocol_state == STATE_IDLE)
        {
            if (rx_byte == 'S')
            {
                protocol_state = STATE_WAITING_DATA;
                rx_index = 0;
            }
            else
            {
                HAL_UART_Transmit(&huart1, &rx_byte, 1, HAL_MAX_DELAY);
            }
        }
        else if (protocol_state == STATE_WAITING_DATA)
        {
            if (rx_index < INPUT_SIZE_BYTES)
            {
                rx_buffer[rx_index++] = rx_byte;
                if (rx_index >= INPUT_SIZE_BYTES)
                {
                    data_ready = 1;
                    protocol_state = STATE_PROCESSING;
                }
            }
        }
        if (!data_ready)
        {
            HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
        }
    }
}
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

#ifdef  USE_FULL_ASSERT
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
