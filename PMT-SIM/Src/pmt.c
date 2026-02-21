/**
  ******************************************************************************
  * @file           : pmt.c
  * @brief          : C source file which stores function's bodies, global variables definitions
  * @author			: Bartosz Rychlicki,
  * 				  Michał Jurek,
  * 				  Michał Gądek
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

/* Includes ------------------------------------------------------------------*/
#include "pmt.h"

/* Variables -----------------------------------------------------------------*/

// HAL' peripherals handles
extern SPI_HandleTypeDef hspi1;						/*<Exported SPI handle>*/

extern TIM_HandleTypeDef htim2;						/*<Exported TIM handle>*/

extern UART_HandleTypeDef huart2;					/*<Exported UART handle>*/

// SPI
uint8_t TxData[3];									/*<TxData buffer for bytes to be send via SPI>*/

uint8_t RxData[3];									/*<RxData buffer for bytes to be received from SPI>*/

const uint8_t AD5360_ADDR_MAP[16] = {				/*<DAC's VOUTs addresses map>*/
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, /*<VOUT0 - VOUT7>*/
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17  /*<VOUT8 - VOUT15>*/
};

// UART
char    rxBuffer[32];     							/*<UART buffer to store received command>*/

uint8_t rxIndex = 0;       							/*<Index which stores, how many chars received from UART>*/

uint8_t tempChar;          							/*ASCII variable, stores single char>*/

char txMessage[64];        						    /*<DAC's response for received command, to send via UART>*/

// RCC
uint32_t then;										/*<lastTick of execution 3rd mode>*/

// PMT-SIM
PMTSIM_TypeDef  pmtSim = {0,0,0,0};			/*<PMT-SIM object>*/

volatile uint8_t selectedDACOutput = 0;		/*<selected Output to write Voltage>*/

volatile float   selectedOUTVoltage = 0;		/*<selected value of Voltage to set on selected DAC's output>*/


/* Functions' bodies ---------------------------------------------------------*/

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

	// returning SPI operation status (can be HAL_OK or HAL_ERROR or HAL_BUSY)
	return status;
}

/*
 * @brief Function that writes 24 bits of command to DAC register
 * */
void AD5361_Write(uint32_t cmd24){

	uint8_t tx[3]; 					// declaration of variable which stores formated 3 bytes of 24-bits command

	// exporting 24-bits command into 3 bytes
	tx[0] = (cmd24 >> 16) & 0xFF;
	tx[1] = (cmd24 >> 8) & 0xFF;
	tx[2] = (cmd24 >> 0) & 0xFF;

	// Enabling overwriting DAC's registers
	HAL_GPIO_WritePin(nSYNC_GPIO_Port, nSYNC_Pin, GPIO_PIN_RESET);

	// sending 3 bytes via SPI
	HAL_SPI_Transmit(&hspi1, tx, 3, HAL_MAX_DELAY);

	// ending operation
	HAL_GPIO_WritePin(nSYNC_GPIO_Port, nSYNC_Pin, GPIO_PIN_SET);
}

/*
 * @brief Function that writes HIGH state to GPIO pin of AD5361
 * */
void AD5361_GPIO_H(void){
	uint32_t cmd = 0;

	// Writing command to set H state on pin GPIO (000011010000000000000011) to 32-bits variable
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

	//Formatting LOW state (000011010000000000000010 in binary) to variable, whose value will be send via SPI
	cmd |= (0x0DUL << 16);
	cmd |= (1U << 1);

	// sending LOW state information to GPIO register of DAC
	AD5361_Write(cmd);

}

/*
 * @brief Function that writes selected voltage to selected output of AD5361
 * */
void AD5360_SetVoltage(float voltage, float v_ref, uint8_t channelIndex) {

    float span  = 4.0f * v_ref;  // calculating absolute range of voltage
    float v_min = -2.0f * v_ref; // calculating lower supply rail border voltage (for 5V it will be -10V)
    float v_max =  2.0f * v_ref; // calculating upper supply rail border voltage (for 5V it will be 10V)

    // Checking if given voltage is in range: [v_min, v_max]
    if (voltage < v_min) voltage = v_min;
    if (voltage > v_max) voltage = v_max;

    // binary code = ((Vout - Vmin) / Span) * MaxCode
    uint16_t dac_code = (uint16_t)(((voltage - v_min) / span) * (DAC_RESOLUTION - 1));

    // Bits 23-22: Mode    (11 -> Write DAC)			 : M1 and M0
    // Bits 21-16: Address (for example: 001000 -> VOUT0): A5  - A0
    // Bits 15-0:  Data    (binary code DAC)			 : D15 - D0
    uint32_t packet = 0;

    packet |= ((uint32_t)AD5360_MODE_WRITE_DAC << 22);   			// Setting mode
    packet |= ((uint32_t)AD5360_ADDR_MAP[channelIndex - 1] << 16);  // Setting target address
    packet |= (dac_code & 0xFFFF);                       			// Setting data, with casting to 16-bits

    // Writing formated packet of data to DAC
    AD5361_Write(packet);

	// Setting LOW state on LDAC to enable SPI writing to DAC's registers
    AD5361_GPIO_L();

	// Delaying firmware to provide time buffer for SPI writing data packet do DAC's register
    for(volatile int i=0; i<10; i++);

    // Turning HIGh state on LDAC to end process
    AD5361_GPIO_H();
}

/*
 * @brief Callback UART functions, that receives and transform received command into embedded operations
 * */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	// Checking if callback was trigged from correct UART instance
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

	                // Cleaning up data buffer of UARTs
	                UART_CleanDataBuffer(huart);

	            }
	        } else {

	            // checking if is there available space, then push_back'ing current char to buffer
	            if (rxIndex < sizeof(rxBuffer) - 1) {
	                rxBuffer[rxIndex++] = tempChar;
	            }
	        }

	        // re-launching interrupts for UART
	        HAL_UART_Receive_IT(huart, &tempChar, 1);
	    }

}

/*
 * @brief Functions that executes HIT command (built for HIT1 and HIT2)
 * */
void PMT_ExecuteHITCommand(PMTSIM_TypeDef* pmt){

	// reading current firmware tick
	static uint32_t now;
	now = HAL_GetTick();

	// state machine, executions depends on chosen mode
	switch (pmt->mode) {
		case NONE:

			// reseting variables, to ensure that after execution of other modes, after re-enabling other mode, old value won't be affect firmware execution
			pmt->frequency = 0;
			pmt->hitId = 0;
			pmt->delayHz = 0;

			break;
		case SINGLESHOT:

			// executing HIT process ( checking if HIT index was given
			  if(pmt->hitId != 0){

				// if HIT1 is selected then write HIGH state on HIT1 , otherwise write HIGH state on HIT2
				HAL_GPIO_WritePin(((pmt->hitId == 1)? hit1_GPIO_Port : hit2_GPIO_Port), ((pmt->hitId == 1)? hit1_Pin : hit2_Pin), 1);

				// delaying 13.4 ns before toggling GPIO state
				delay_ns(1);

				// if HIT1 is selected then write LOW state on HIT1 , otherwise write LOW state on HIT2
				HAL_GPIO_WritePin(((pmt->hitId == 1)? hit1_GPIO_Port : hit2_GPIO_Port), ((pmt->hitId == 1)? hit1_Pin : hit2_Pin), 0);

				// reseting hitId flag
				pmt->hitId = 0;

				// reseting current selected mode
				pmt->mode = NONE;


			  }
			  break;
		case SINGLESHOT_DELAY:

			// Init of variable, which will stores delay between HIT1 and HIT2 in [ns]
			uint32_t gap_ns = 0;

			// converting delay from [Hz] -> [ns]
			gap_ns = 1000000000 / pmtSim.delayHz;


			/*
			// subtraction of firmware delay
			if(gap_ns > 4000){
				gap_ns -= 3500;
			}
			*/


			/*
			 * Executing HIT1
			 * */

			// Writing HIGH state to HIT1 pin
			HAL_GPIO_WritePin(hit1_GPIO_Port, hit1_Pin, GPIO_PIN_SET);

			// delaying 13.4 ns before toggling state on HIT1's pin
			delay_ns(1);

			// Writing LOW state to HIT1 pin
			HAL_GPIO_WritePin(hit1_GPIO_Port, hit1_Pin, GPIO_PIN_RESET);


			// executing delay between HIT1 and HIT2
			delay_ns(gap_ns);

			/*
			 * Executing HIT2
			 * */

			// Writing HIGH state to HIT2 pin
			HAL_GPIO_WritePin(hit2_GPIO_Port, hit2_Pin, GPIO_PIN_SET);

			// delaying 13.4 ns before toggling state on HIT2's pin
			delay_ns(1);

			// Writing LOW state to HIT2 pin
			HAL_GPIO_WritePin(hit2_GPIO_Port, hit2_Pin, GPIO_PIN_RESET);

			// reseting current mode
			pmt->mode = NONE;


			break;
		case SINGLESHOT_DELAY_FREQ:
		    {
		    	// reading current firmware's tick [ms]
		        now = HAL_GetTick();

		        // Preventing from dividing by 0
		        if (pmtSim.frequency == 0) break;

		        // Calculating period between whole operations (HIT1 and HIT2 which delay in [ns])
		        uint32_t period_ns = 1000000 / pmtSim.frequency;

		        // if calculated delay is too low, then firmware provides minimum value
		        if (period_ns == 0) period_ns = 1;

		        // converting 'now' and 'then' ticks from [ms] -> [ns], and checing if correct amout of time has passed
		        if (1000*(now - then) >= period_ns)
		        {

		        	// saving current tick
		            then = now;


		            // calculating delay between HIT1 ad HIT2 in [ns]
		            uint32_t gap_ns = 1000000000 / pmtSim.delayHz;

		            // removing firmware delay
		            if(gap_ns > 4000){
						gap_ns -= 4000;
					}


		            /*
		             * 	Executing HIT1
		             * */

		            // Writing HIGH state to HIT1's pin
		            HAL_GPIO_WritePin(hit1_GPIO_Port, hit1_Pin, GPIO_PIN_SET);

		            // delaying 13.4 ns before toggling state of HIT1 pin
		            delay_ns(1);

		            // Writing LOW state to HIT1's pin
		            HAL_GPIO_WritePin(hit1_GPIO_Port, hit1_Pin, GPIO_PIN_RESET);


		            // Delay Between HIT1 and HIT2
		            delay_ns(gap_ns);


		            /*
					 * 	Executing HIT2
					 * */

		            // Writing HIGH state to HIT2's pin
		            HAL_GPIO_WritePin(hit2_GPIO_Port, hit2_Pin, GPIO_PIN_SET);

		            // delaying 13.4 ns before toggling state of HIT2 pin
		            delay_ns(1);

		            // Writing LOW state to HIT2's pin
		            HAL_GPIO_WritePin(hit2_GPIO_Port, hit2_Pin, GPIO_PIN_RESET);


		        }
		    }
		    break;
		default:
			break;
	}

}

/*
 * @brief Functions that does clean-up of UART's data register after executed operation
 * */
void UART_CleanDataBuffer(UART_HandleTypeDef* huart){

	// deleting all data from buffer
	huart->Instance->DR &= 0U;

}

/*
 * @brief Functions that convert UART command into fimware's operations
 * */
HAL_StatusTypeDef UART_ConvertCommand(char* line){


	// Checking if given command is co-related to HIT command: "H;<n>;DELAY;FREQ"
	if (line[0] == 'H' && line[1] == ';')
	{
		// Init of variable which will store received parameters
		char hitId_c[15];		// selected HIT container
		char delay_c[15];		// received delay (in [Hz]) container
		char freq_c[15];		// received frequency of 3rd mode (in [Hz]) container

		// reading params from command buffer (command received via UART) to correct params' buffers
		sscanf(line, "%*[^;];%[^;];%[^;];%[^\r\n]",hitId_c, delay_c, freq_c);

		// converting received params from char arrays into uint and float types
		pmtSim.hitId = (uint8_t)atoi(hitId_c);
		pmtSim.delayHz = (uint32_t)atoi(delay_c);
		pmtSim.frequency = (float)atof(freq_c);


		/*
		 * Selecting mode by checking values of received params
		*/

		// Checking If operations should not be periodic
		if(pmtSim.frequency == 0){

			// Checking if delay is provided
			if(pmtSim.delayHz  == 0){

				pmtSim.mode = SINGLESHOT;

			}else{
				pmtSim.mode = SINGLESHOT_DELAY;
			}

		}else{
			pmtSim.mode = SINGLESHOT_DELAY_FREQ;
		}

		// returning status
		return HAL_OK;

	}
		// Oterhwise command is co-related to setting voltage on one of DAC's outputs: <id>;<value>
		else
	{

		//checking on with address is semicolon
		char *semi = strchr(line, ';');

		// checking if semicolon pointer is not NULL
		if (semi != NULL) {

			// divining string into two lines and changing semicolon to '\0' to separates two params
			*semi = '\0';

			// converting received number of OUTPUT (1-6) from first separated line to uin8_t data type
			selectedDACOutput = (uint8_t)atoi(line);

			// checking if received number of output is correct
			if(selectedDACOutput > 6 || selectedDACOutput < 1){
				return HAL_ERROR;
			}

			// moving to next address (second extracted line)
			char *val_str = semi + 1;
			char *endp = NULL;			// init of variable which stores "end of line" address

			// reading selected voltage from address of char with stand after semicolon,(semicolon address + 1) to end of line and converting into float type of data
			selectedOUTVoltage = strtof(val_str, &endp);
		}
	}

	return HAL_OK;
}

/*
 * @brief Functions that returns ACK information, co-related to received UART comman
 * */
HAL_StatusTypeDef UART_ReturnCommand(char* line){

	// adding ACK status of received command
	sprintf(txMessage, "ACK:%s OK\r\n", line);

	// sending response to GUI
    if(HAL_UART_Transmit(&huart2, (uint8_t*)txMessage, strlen(txMessage), HAL_MAX_DELAY) != HAL_OK){
        return HAL_ERROR;
    }

    return HAL_OK;
}

/*
 * @brief Functions that provides delay in [ns]
 * */
void delay_ns(uint16_t ns)
{
	uint32_t ticks = (ns * 72 + 999) / 1000; 		// rounding up ticks number | adding 999 exist to provide non-zero amount of ticks (because otherwise firmware never exits this function)

	__HAL_TIM_SET_COUNTER(&htim2, 0);				// reseting TIM counter
	while (__HAL_TIM_GET_COUNTER(&htim2) < ticks);	// while loop that executes as long as TIM counts to calculated ticks
}

