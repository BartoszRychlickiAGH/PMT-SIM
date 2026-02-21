/**
  ******************************************************************************
  * @file           : pmt.h
  * @brief          : Header for pmt.c file.
  *                   This file contains the common defines of the application, function's prototypes and typedefs definitions.
  * @author			: Bartosz Rychlicki,
  * 				  Michał Jurek,
  * 				  Michał Gądek
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 AGH University of Science and Technology.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef PMT_H_
#define PMT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"
#include "inttypes.h"
#include "ctype.h"
#include <stdlib.h>
#include <stdio.h>
/* Typedefs definitions-------------------------------------------------------*/

/**
  * @brief  PMT-SIM's chosen mode enumeration type definition
  */
typedef enum{
	NONE  = 0,									/*<Default mode, when none of presented below is chosen>*/
	SINGLESHOT,									/*<Single shot, when HIT1 or HIT2 operation should be performed>*/
	SINGLESHOT_DELAY,							/*<Single shot with delay, when HIT1 and HIT2 operation must be performed one after the other with delay>*/
	SINGLESHOT_DELAY_FREQ						/*<Periodic Single shot with delay operation, when hit1 and hit2 should be performed with delay and the whole operation must be repeated cyclically>*/
}PMTSIM_ModeTypeDef;

/**
  * @brief  PMT-SIM Structure definition
  */
typedef struct{

	PMTSIM_ModeTypeDef mode;					/*<Mode of current workflow>*/

	uint8_t  hitId;								/*<Current id of hit>*/
	uint32_t delayHz;							/*<Phase delay between HIT1 and HIT2>*/
	float frequency;							/*<Frequency of HIT operation in third mode>*/

}PMTSIM_TypeDef;

/* Macros --------------------------------------------------------------------*/
#define AD5360_MODE_WRITE_DAC       0x3 	 	/*<M0 and M1 bits' values:  M1 = 1, M0 = 1>*/

#define DAC_RESOLUTION              65536.0f 	/*<16-bits DAC's resolution>*/

/* Variables -----------------------------------------------------------------*/
extern SPI_HandleTypeDef hspi1;					/*<Exported SPI handle>*/

extern TIM_HandleTypeDef htim2;					/*<Exported TIM handle>*/

extern UART_HandleTypeDef huart2;				/*<Exported UART handle>*/

extern PMTSIM_TypeDef  pmtSim;					/*<Exported PMT-SIM object>*/

extern uint8_t tempChar;						/*<Exported received char from UART>*/

extern uint32_t then;							/*<Exported lastTick of 3rd mode>*/

extern volatile uint8_t selectedDACOutput;		/*<Exported selected Output to write Voltage>*/

extern volatile float   selectedOUTVoltage;		/*<Exported selected value of Voltage to set on selected DAC's output>*/

/* Function prototypes -------------------------------------------------------*/

void UART_CleanDataBuffer(UART_HandleTypeDef* huart);

HAL_StatusTypeDef UART_ConvertCommand(char* line);

HAL_StatusTypeDef UART_ReturnCommand(char* line);

HAL_StatusTypeDef SPI_SendFrame(uint8_t tx_data[3]);

void AD5361_Write(uint32_t cmd24);

void AD5361_GPIO_H(void);

void AD5361_GPIO_L(void);

void AD5360_SetVoltage(float voltage, float v_ref, uint8_t channelIndex);

void PMT_ExecuteHITCommand(PMTSIM_TypeDef* pmt);

void delay_ns(uint16_t us);


#ifdef __cplusplus
}
#endif

#endif /* PMT_H_ */
