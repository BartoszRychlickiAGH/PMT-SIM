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
#include "string.h"
#include "inttypes.h"
#include "ctype.h"
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define AD5360_MODE_WRITE_DAC       0x3 	 //  M0 and M1 bits' values:  M1 = 1, M0 = 1

#define AD5360_ADDR_GROUP0_CH0      0x08     // address of out0 | just for test cases

#define DAC_RESOLUTION              65536.0f // 16-bits DAC's resolution
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t TxData[3];
uint8_t RxData[3];


// Buffers declaration
char    rxBuffer[32];     							// UART buffer to stores received command
uint8_t rxIndex = 0;       							// Index which stores, how many chars received from UART
uint8_t tempChar;          							// ASCII variable, stores single char

char txMessage[64];        						    // DAC's response for received command, to send via UART


const uint8_t AD5360_ADDR_MAP[16] = {				// DAC's VOUTs addresses map
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // VOUT0 - VOUT7
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17  // VOUT8 - VOUT15
};

static volatile uint8_t hit_id = 0;					// selected HIT index | if none HIT is selected, then variable stores 0

static volatile uint8_t selectedDACOutput = 0;		// selected Output to write Voltage
static volatile float   selectedOUTVoltage = 0;		// selected value of Voltage to set on selected DAC's output
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

HAL_StatusTypeDef UART_ConvertCommand(char command [5]);

HAL_StatusTypeDef UART_ReturnCommand(char line [5]);

HAL_StatusTypeDef SPI_SendFrame(uint8_t tx_data[3]);

void AD5361_Write(uint32_t cmd24);

void AD5361_GPIO_H(void);

void AD5361_GPIO_L(void);

void AD5360_SetVoltage_VOUT(float voltage, float v_ref, uint8_t channelIndex);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

  // Enabling interrupts for UART (implementation on DMA is possible)

  if(HAL_UART_Receive_IT(&huart2, &tempChar, 1) != HAL_OK){
	  Error_Handler();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */



  // setting default state on LDAC pin of DAC (it's wired to GPIO pin by GPIO wire)
  AD5361_GPIO_H();

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


	  	  // executing HIT process
	  	  if(hit_id != 0){

	  		// if HIT1 is selected then write HIGH state on HIT1 , otherwise write HIGH state on HIT2
	  		HAL_GPIO_WritePin(((hit_id == 1)? hit1_GPIO_Port : hit2_GPIO_Port), ((hit_id == 1)? hit1_Pin : hit2_Pin), 1);

			HAL_Delay(500);

			// if HIT1 is selected then write LOW state on HIT1 , otherwise write LOW state on HIT2
			HAL_GPIO_WritePin(((hit_id == 1)? hit1_GPIO_Port : hit2_GPIO_Port), ((hit_id == 1)? hit1_Pin : hit2_Pin), 0);

			hit_id = 0; // reseting hit_id flag, to prevent multiple executing

	  	  }else if(selectedDACOutput != 0){	// writing selected voltage value to selected DAC's output

	  		AD5360_SetVoltage_VOUT0(selectedOUTVoltage, 5.0f, selectedDACOutput);

	  		selectedDACOutput = 0; // reseting selectedDACOutput flag

	  	  }

  /* USER CODE END 3 */
  }
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

/*
 * @brief Function that sends 24 bits to AD5361's register
 * */
HAL_StatusTypeDef SPI_SendFrame(uint8_t tx_data[3]){
	HAL_StatusTypeDef status;

	// turning nSYNC pin to sleep (according to datasheet, in this time we can overwrite registers via SPI)
	HAL_GPIO_WritePin(nSYNC_GPIO_Port, nSYNC_Pin, GPIO_PIN_RESET);

	// sending 3 bytes via SPI
	status  = HAL_SPI_TransmitReceive(&hspi1, tx_data, RxData, 3, HAL_MAX_DELAY);

	// ending writing process by turning nSYNC to 3V3 level
	HAL_GPIO_WritePin(nSYNC_GPIO_Port, nSYNC_Pin, GPIO_PIN_SET);

	return status;
}

/*
 * @brief Function that writes 24 bits of command to DAC register
 * */
void AD5361_Write(uint32_t cmd24){

	uint8_t tx[3]; 					// declaration of variable which stores formated 3 bytes of 24-bits command

	tx[0] = (cmd24 >> 16) & 0xFF;
	tx[1] = (cmd24 >> 8) & 0xFF;
	tx[2] = (cmd24 >> 0) & 0xFF;

	// Enabling overwriting DAC's registers
	HAL_GPIO_WritePin(nSYNC_GPIO_Port,nSYNC_Pin,GPIO_PIN_RESET);

	// sending 3 bytes via SPI
	HAL_SPI_Transmit(&hspi1,tx,3,HAL_MAX_DELAY);

	// ending operation
	HAL_GPIO_WritePin(nSYNC_GPIO_Port,nSYNC_Pin,GPIO_PIN_SET);
}

/*
 * @brief Function that writes HIGH state to GPIO pin of AD5361
 * */
void AD5361_GPIO_H(void){
	uint32_t cmd = 0;

	//Writting this binary value: 000011010000000000000011 to 32-bits varianle
	cmd |= (0x0DUL << 16);
	cmd |= (1U << 1);
	cmd |= (1U << 0);

	// sending binary value to DAC's GPIO register
	AD5361_Write(cmd);
}

/*
 * @brief Function that writes LOW state to GPIO pin of AD5361
 * */
void AD5361_GPIO_L(void){
	uint32_t cmd = 0;

	//Formatting LOW state (000011010000000000000010 in binary) to variable, which its value will be send via SPI
	cmd |= (0x0DUL << 16);
	cmd |= (1U << 1);

	// sending LOW state information to GPIO register of DAC
	AD5361_Write(cmd);

}


void AD5360_SetVoltage_VOUT(float voltage, float v_ref, uint8_t channelIndex) {

    float span  = 4.0f * v_ref;  // calculating absolute range of voltage
    float v_min = -2.0f * v_ref; // calculating lower supply rail border voltage (for 5V it will be -10V)
    float v_max =  2.0f * v_ref; // calculating upper supply rail border voltage (for 5V it will be 10V)

    // Checking if given voltage is in range of power supply rails
    if (voltage < v_min) voltage = v_min;
    if (voltage > v_max) voltage = v_max;

    // binary code = ((Vout - Vmin) / Span) * MaxCode
    uint16_t dac_code = (uint16_t)(((voltage - v_min) / span) * (DAC_RESOLUTION - 1));

    // Bits 23-22: Mode    (11 -> Write DAC)
    // Bits 21-16: Address (001000 -> VOUT0)
    // Bits 15-0:  Data    (binary code DAC)
    uint32_t packet = 0;

    packet |= ((uint32_t)AD5360_MODE_WRITE_DAC << 22);   		// Setting mode
    packet |= ((uint32_t)AD5360_ADDR_MAP[channelIndex] << 16);  // Setting target address
    packet |= (dac_code & 0xFFFF);                       		// Setting data

    // Writing formated packet of data to DAC
    AD5361_Write(packet);

	// Setting LOW state on LDAC to enable SPI writing to DAC's registers
    AD5361_GPIO_L();

	// Delaying firmware to provide time buffer for SPI writing data packet do DAC's register
    for(volatile int i=0; i<10; i++);

    // Turning HIGh state on LDAC to end process
    AD5361_GPIO_H();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

	if (huart->Instance == USART2) {

	        // checking if current character is an end of command
	        if (tempChar == '\n' || tempChar == '\r') {
	            if (rxIndex > 0) {
	                rxBuffer[rxIndex] = '\0'; // closing and saving line

	                // converting received command and overwriting correct flags
	                UART_ConvertCommand(rxBuffer);

	                // sending response to GUI
	                UART_ReturnCommand(rxBuffer);

	                rxIndex = 0; // reseting buffer for next command
	            }
	        } else {

	            // checking if is there available space, then push_backing char to line
	            if (rxIndex < sizeof(rxBuffer) - 1) {
	                rxBuffer[rxIndex++] = tempChar;
	            }
	        }

	        // re-launchinf interrupts for UART
	        HAL_UART_Receive_IT(huart, &tempChar, 1);
	    }

}


HAL_StatusTypeDef UART_ConvertCommand(char* line){

	// HIT: "H;<n>"
	if (line[0] == 'h' && line[1] == ';')
	{
		// converting received id of HIT to number type
		hit_id = (uint8_t)atoi(&line[2]);
		// HitAction(hit_id);
		return HAL_OK;
	}

	// Slider: "<id>;<value>"
	char *semi = strchr(line, ';');	//checking on with address is semicolon

	if (semi != NULL) {	//

		// divining string into two lines and changing semicolon to '\0'
		*semi = '\0';

		// reading received number of OUTPUT (1-6)
		selectedDACOutput = (uint8_t)atoi(line);

		// checking if received number of output is correct
		if(selectedDACOutput > 6 || selectedDACOutput < 1){
			return HAL_ERROR;
		}

		// moving to next address (second extracted line)
 		char *val_str = semi + 1;
		char *endp = NULL;

		// reading selected voltage from semicolon to end of line and converting into float type of data
		selectedOUTVoltage = strtof(val_str, &endp);

	}


	return HAL_OK;
}

HAL_StatusTypeDef UART_ReturnCommand(char* line){

	// adding ACl status of received command
	sprintf(txMessage, "ACK:%s OK\r\n", line);

	// sending response to GUI
    if(HAL_UART_Transmit(&huart2, (uint8_t*)txMessage, strlen(txMessage), HAL_MAX_DELAY) != HAL_OK){
        return HAL_ERROR;
    }

    return HAL_OK;
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
	  char debug[32] = "Error handler triggered";


	  UNUSED(debug);;
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
