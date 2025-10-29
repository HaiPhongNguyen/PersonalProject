/*
 * Pzem004T.h
 *
 *  Created on: Oct 15, 2025
 *      Author: Phong
 */

#ifndef INC_PZEM004T_H_
#define INC_PZEM004T_H_

#include "stm32f1xx_hal.h"

#define PZEM_VOL_REG						0x00
#define PZEM_CUR_LOW_REG					0x01
#define PZEM_CUR_HIGH_REG					0x02
#define PZEM_POW_LOW_REG					0x03
#define PZEM_POW_HIGH_REG					0x04
#define PZEM_ENERGY_LOW_REG					0x05
#define PZEM_ENERGY_HIGH_REG				0x06
#define PZEM_FREQ_REG						0x07

void Pzem_Init(UART_HandleTypeDef *uart);
HAL_StatusTypeDef Pzem_ReadVoltage(float *voltage);
HAL_StatusTypeDef Pzem_ReadCurrent(float *current);
HAL_StatusTypeDef Pzem_ReadPower(float *power);
HAL_StatusTypeDef Pzem_ReadEnergy(float *energy);
HAL_StatusTypeDef Pzem_ReadFreq(float *freq);

#endif /* INC_PZEM004T_H_ */
