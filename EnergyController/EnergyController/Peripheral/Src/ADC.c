/*
 * ADC.c
 *
 *  Created on: Oct 12, 2025
 *      Author: Phong
 */

#include "ADC.h"
#include "main.h"
#include "cmsis_os.h"

static ADC_HandleTypeDef *hadc;

/* Thông số cầu chia áp */
#define R1_KOHM    100.0f
#define R2_KOHM    33.0f
#define VREF       3.3f
#define ADC_RES    4096.0f  // 12-bit ADC

void ADC_Init(ADC_HandleTypeDef *xADC)
{
	hadc = xADC;
}

HAL_StatusTypeDef ADC_Read(float *voltage)
{
	HAL_StatusTypeDef eStatus = HAL_OK;

	eStatus |= HAL_ADC_Start(hadc);
	eStatus |= HAL_ADC_PollForConversion(hadc, 1000U);

	if (eStatus == HAL_OK)
	{
		uint16_t raw = HAL_ADC_GetValue(hadc);

		/* Tính điện áp thực tế tại chân ADC */
		float v_adc = (raw * VREF) / ADC_RES;

		/* Tính điện áp trước cầu chia (điện áp pin) */
		*voltage = v_adc * ((R1_KOHM + R2_KOHM) / R2_KOHM) * 12;
		*voltage /= 11;
	}

	eStatus |= HAL_ADC_Stop(hadc);

	return eStatus;
}

