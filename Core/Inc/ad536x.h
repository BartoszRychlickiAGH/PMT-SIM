/* ad536x.h */

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define AD536X_CMD_WRITE_UPDATE_DAC 0x3


typedef struct {
    SPI_HandleTypeDef *hspi;

    GPIO_TypeDef *CS_Port;
    uint16_t CS_Pin;


    GPIO_TypeDef *hit1_port;
    uint16_t hit1_pin;
    GPIO_TypeDef *hit2_port;
    uint16_t hit2_pin;

} AD536x_t;


void AD536x_Init(AD536x_t *dac, SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin,
                 GPIO_TypeDef *hit1_port, uint16_t hit1_pin,
                 GPIO_TypeDef *hit2_port, uint16_t hit2_pin);

HAL_StatusTypeDef AD536x_Write(AD536x_t *dac, uint8_t command, uint8_t address, uint16_t data);

HAL_StatusTypeDef AD536x_SetCode(AD536x_t *dac, uint8_t channel, uint16_t dacCode);


void AD536x_Pulse_hit1(AD536x_t *dac);
void AD536x_Pulse_hit2(AD536x_t *dac);
