/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define AD5360_MODE_WRITE_DAC       0x3 	 //Wartość bitów M0 i M1:  M1 = 1, M0 = 1
#define AD5360_ADDR_GROUP0_CH0      0x08     // Adres w rejestrze DAC dla wyjścia VOUT0: A5-A0 = 001000
#define DAC_RESOLUTION              65536.0f // Rozdzielczość 16-bitowego DAC: 2^16
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t TxData[3];
uint8_t RxData[3];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


HAL_StatusTypeDef SPI_SendFrame(uint8_t tx_data[3]){
	HAL_StatusTypeDef status;

	HAL_GPIO_WritePin(nSYNC_GPIO_Port, nSYNC_Pin, GPIO_PIN_RESET);

	status  = HAL_SPI_TransmitReceive(&hspi1, tx_data, RxData, 3, HAL_MAX_DELAY);

	HAL_GPIO_WritePin(nSYNC_GPIO_Port, nSYNC_Pin, GPIO_PIN_SET);

	return status;
}


void AD5361_Write(uint32_t cmd24){
	uint8_t tx[3];
	tx[0] = (cmd24 >> 16) & 0xFF;
	tx[1] = (cmd24 >> 8) & 0xFF;
	tx[2] = (cmd24 >> 0) & 0xFF;
	HAL_GPIO_WritePin(nSYNC_GPIO_Port,nSYNC_Pin,GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi1,tx,3,HAL_MAX_DELAY);
	HAL_GPIO_WritePin(nSYNC_GPIO_Port,nSYNC_Pin,GPIO_PIN_SET);
}

void AD5361_GPIO_H(void){
	uint32_t cmd = 0;

	//000011010000000000000011
	cmd |= (0x0DUL << 16);
	cmd |= (1U << 1);
	cmd |= (1U << 0);


	AD5361_Write(cmd);
}
void AD5361_GPIO_L(void){
	uint32_t cmd = 0;

	//000011010000000000000010
	cmd |= (0x0DUL << 16);
	cmd |= (1U << 1);


	AD5361_Write(cmd);

}

void AD5360_SetVoltage_VOUT0(float voltage, float v_ref) {

    float span = 4.0f * v_ref;
    float v_min = -2.0f * v_ref; // Np. -10V dla Vref = 5V
    float v_max =  2.0f * v_ref; // Np. +10V dla Vref = 5V

    // Sprawdzenie czy podane napięcie jest w zakresie
    if (voltage < v_min) voltage = v_min;
    if (voltage > v_max) voltage = v_max;

    // kod = ((Vout - Vmin) / Span) * MaxCode
    uint16_t dac_code = (uint16_t)(((voltage - v_min) / span) * (DAC_RESOLUTION - 1));

    // Bity 23-22: Tryb (11 -> Zapis DAC)
    // Bity 21-16: Adres (001000 -> VOUT0)
    // Bity 15-0:  Dane (Kod DAC)
    uint32_t packet = 0;

    packet |= ((uint32_t)AD5360_MODE_WRITE_DAC << 22);   // Ustawienie trybu
    packet |= ((uint32_t)AD5360_ADDR_GROUP0_CH0 << 16);  // Ustawienie adresu
    packet |= (dac_code & 0xFFFF);                       // Ustawienie danych

    // Zapis pakietu do rejestru
    AD5361_Write(packet);

-	// Wystawienie na pin LDAC niskiego stanu
    AD5361_GPIO_L();

	// Opoźnienie w celu zapisania danych w rejestrze
    for(volatile int i=0; i<10; i++);

    // Wystawienie na pin LDAC wysokiego stanu, aby zakończyć operacje
    AD5361_GPIO_H();
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
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  // Ustawienie domyślnego stanu na pinie LDAC (połączonego przewodem z pinem GPIO)
  AD5361_GPIO_H();

  while (1)
  {
      //AD5360_SetVoltage_VOUT0(2.5f, 5.0f);
      //HAL_Delay(1000);

      //AD5360_SetVoltage_VOUT0(-5.0f, 5.0f);
      //HAL_Delay(1000);

      //AD5360_SetVoltage_VOUT0(0.0f, 5.0f);
      //HAL_Delay(1000);

	  // Ustawienie napięcie 5V na wyjściu Vout0
      AD5360_SetVoltage_VOUT0(5.0f, 5.0f);
	  HAL_Delay(3000);


	  /*
	   * Rozładowanie kondensatora
	   */

	  HAL_GPIO_WritePin(hit1_GPIO_Port, hit1_Pin, 1);

	  HAL_Delay(1500);

	  HAL_GPIO_WritePin(hit1_GPIO_Port, hit1_Pin, 0);


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
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
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
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  HAL_GPIO_WritePin(GPIOB, hit1_Pin|hit2_Pin|nSYNC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : hit1_Pin hit2_Pin nSYNC_Pin */
  GPIO_InitStruct.Pin = hit1_Pin|hit2_Pin|nSYNC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

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
