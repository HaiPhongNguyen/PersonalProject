/*
 * read_sensor.h
 *  Created on: Oct 18, 2025
 *      Author: Phong
 */

#ifndef INC_READ_SENSOR_H_
#define INC_READ_SENSOR_H_

#include "stm32f1xx_hal.h"
#include "INA219.h"
#include "Pzem004T.h"

typedef struct
{
    float solarVoltage;     // Điện áp tấm pin mặt trời (V)
    float solarCurrent;     // Dòng tấm pin mặt trời (A)
    float turbinVoltage;    // Điện áp máy phát gió (V)
    float turbinCurrent;    // Dòng máy phát gió (A)
    float batteryVoltage;   // Điện áp pin lưu trữ (V)
} SensorData_t;

typedef struct
{
	float port1Voltage;
	float port2Voltage;
	float port3Voltage;
	float port4Voltage;
	float IvVoltage;
	float port1Current;
	float port2Current;
	float port3Current;
	float port4Current;
	float IvCurrent;
} SourceOutputData_t;

void Sensor_Init(I2C_HandleTypeDef *hi2c_ads, ADC_HandleTypeDef *hadc_in);
void Sensor_ReadAll(SensorData_t *data);
void Sensor_Calibrate(uint16_t sampleCount);
void SourceOutput_Init(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart);
void SourceOutput_ReadAll(SourceOutputData_t *data);
#endif /* INC_READ_SENSOR_H_ */
