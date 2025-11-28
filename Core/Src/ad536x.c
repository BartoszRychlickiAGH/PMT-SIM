/* ad536x.c */

#include "ad536x.h"


void AD536x_Init(AD536x_t *dac, SPI_HandleTypeDef *hspi,
                 GPIO_TypeDef *cs_port, uint16_t cs_pin,
                 GPIO_TypeDef *hit1_port, uint16_t hit1_pin,
                 GPIO_TypeDef *hit2_port, uint16_t hit2_pin) {

    dac->hspi = hspi;

    dac->CS_Port = cs_port;
    dac->CS_Pin = cs_pin;

     dac->hit1_port = hit1_port;
    dac->hit1_pin = hit1_pin;

    dac->hit2_port = hit2_port;
    dac->hit2_pin = hit2_pin;


    HAL_GPIO_WritePin(dac->CS_Port, dac->CS_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(dac->hit1_port, dac->hit1_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(dac->hit2_port, dac->hit2_pin, GPIO_PIN_RESET);
}


HAL_StatusTypeDef AD536x_Write(AD536x_t *dac, uint8_t command, uint8_t address, uint16_t data) {
    HAL_StatusTypeDef status;

    uint32_t word = ((uint32_t)command << 20) |
                    ((uint32_t)address << 16) |
                    (uint32_t)data;

    uint8_t tx_data[3];
    tx_data[0] = (uint8_t)(word >> 16);
    tx_data[1] = (uint8_t)(word >> 8);
    tx_data[2] = (uint8_t)(word);

    HAL_GPIO_WritePin(dac->CS_Port, dac->CS_Pin, GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(dac->hspi, tx_data, 3, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(dac->CS_Port, dac->CS_Pin, GPIO_PIN_SET);

    return status;
}

HAL_StatusTypeDef AD536x_SetCode(AD536x_t *dac, uint8_t channel, uint16_t dacCode) {
    return AD536x_Write(dac, AD536X_CMD_WRITE_UPDATE_DAC, channel, dacCode);
}


void AD536x_Pulse_hit1(AD536x_t *dac) {
    HAL_GPIO_WritePin(dac->hit1_port, dac->hit1_pin, GPIO_PIN_SET); // Użycie pól z małymi literami
    for(volatile int i=0; i<100; i++);
    HAL_GPIO_WritePin(dac->hit1_port, dac->hit1_pin, GPIO_PIN_RESET);
}

void AD536x_Pulse_hit2(AD536x_t *dac) {
    HAL_GPIO_WritePin(dac->hit2_port, dac->hit2_pin, GPIO_PIN_SET); // Użycie pól z małymi literami
    for(volatile int i=0; i<100; i++);
    HAL_GPIO_WritePin(dac->hit2_port, dac->hit2_pin, GPIO_PIN_RESET);
}
