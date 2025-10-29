#ifndef __INA219_H__
#define __INA219_H__

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* ====== INA219 Register Map ====== */
#define INA_CFG_REG						0x00
#define INA_SHT_VOL_REG					0x01
#define INA_BUS_VOL_REG					0x02
#define INA_PWR_REG						0x03
#define INA_CUR_REG 					0x04
#define INA_CALIB_REG					0x05

/* ====== Configuration bits ====== */
#define INA_CFG_RST_BIT					(0b1 << 15)
#define INA_CFG_BRNG_BIT				(0b1 << 13)   // 0 = 16V, 1 = 32V range
#define INA_CFG_PG_BITS(x)				((x & 0x03) << 11) // Gain
#define INA_CFG_BADC_BITS(x)			((x & 0x0F) << 7)  // Bus ADC resolution
#define INA_CFG_SADC_BITS(x)			((x & 0x0F) << 3)  // Shunt ADC resolution
#define INA_CFG_MODE_BITS(x)			(x & 0x07)

/* ====== Operating Modes ====== */
#define INA_MODE_POWER_DOWN				0x00
#define INA_MODE_SHUNT_TRIG				0x01
#define INA_MODE_BUS_TRIG				0x02
#define INA_MODE_SHUNT_BUS_TRIG			0x03
#define INA_MODE_ADC_OFF				0x04
#define INA_MODE_SHUNT_CONT				0x05
#define INA_MODE_BUS_CONT				0x06
#define INA_MODE_SHUNT_BUS_CONT			0x07

/* ====== Struct Handle ====== */
typedef struct {
	I2C_HandleTypeDef *hi2c;
	uint8_t address;
	float shunt_resistance;   // Ω
	float current_lsb;        // A/bit
	float power_lsb;          // W/bit
	uint16_t calib_value;
} INA219_HandleTypeDef;

/* ====== Function prototypes ====== */
HAL_StatusTypeDef INA219_Init(INA219_HandleTypeDef *ina);
HAL_StatusTypeDef INA219_Reset(INA219_HandleTypeDef *ina);
HAL_StatusTypeDef INA219_SetCalibration(INA219_HandleTypeDef *ina, float r_shunt, float i_max);

float INA219_ReadBusVoltage(INA219_HandleTypeDef *ina);
float INA219_ReadShuntVoltage(INA219_HandleTypeDef *ina);
float INA219_ReadCurrent(INA219_HandleTypeDef *ina);
float INA219_ReadPower(INA219_HandleTypeDef *ina);

#endif /* __INA219_H__ */
