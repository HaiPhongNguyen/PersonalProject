/*
 * ADC.h
 *
 *  Created on: Oct 12, 2025
 *      Author: Phong
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_

#include "stm32f1xx_hal.h"

/*=====================================================================
 * CONFIG OPTIONS
 *====================================================================*/

void ADC_Init(ADC_HandleTypeDef *xADC);
HAL_StatusTypeDef ADC_Read(float *voltage);

#endif /* INC_ADC_H_ */
