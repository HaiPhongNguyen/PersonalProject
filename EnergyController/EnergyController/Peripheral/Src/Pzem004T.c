/*
 * Pzem004T.c
 * Author: Phong
 */

#include "Pzem004T.h"
#include "stm32f1xx_hal.h"

static UART_HandleTypeDef *huart;

/* ==== Modbus Commands ==== */
const uint8_t PZEM_CMD_VOLTAGE[8]      = {0x01, 0x04, 0x00, 0x00, 0x00, 0x01, 0x31, 0xCA};
const uint8_t PZEM_CMD_CURRENT[8]      = {0x01, 0x04, 0x00, 0x01, 0x00, 0x02, 0x21, 0xCA};
const uint8_t PZEM_CMD_POWER[8]        = {0x01, 0x04, 0x00, 0x03, 0x00, 0x02, 0x80, 0x0A};
const uint8_t PZEM_CMD_ENERGY[8]       = {0x01, 0x04, 0x00, 0x05, 0x00, 0x02, 0x60, 0x0B};
const uint8_t PZEM_CMD_FREQUENCY[8]    = {0x01, 0x04, 0x00, 0x07, 0x00, 0x01, 0xB0, 0x09};
const uint8_t PZEM_CMD_POWER_FACTOR[8] = {0x01, 0x04, 0x00, 0x08, 0x00, 0x01, 0xE1, 0xC8};

void Pzem_Init(UART_HandleTypeDef *uart)
{
	huart = uart;
}

/* ==== Read Voltage ==== */
HAL_StatusTypeDef Pzem_ReadVoltage(float *voltage)
{
	HAL_StatusTypeDef eStatus;
	uint8_t rx_buf[32];

	eStatus = HAL_UART_Transmit(huart, (uint8_t*)PZEM_CMD_VOLTAGE, sizeof(PZEM_CMD_VOLTAGE), 100);
	if(eStatus != HAL_OK) return eStatus;

	eStatus = HAL_UART_Receive(huart, rx_buf, 32, 200);
	if((eStatus == HAL_OK) && (rx_buf[1] == 0x04))
	{
		uint16_t val = (rx_buf[3] << 8) | rx_buf[4];
		*voltage = val * 0.1f;
	}
	return eStatus;
}

/* ==== Read Current ==== */
HAL_StatusTypeDef Pzem_ReadCurrent(float *current)
{
	HAL_StatusTypeDef eStatus;
	uint8_t rx_buf[32];

	eStatus = HAL_UART_Transmit(huart, (uint8_t*)PZEM_CMD_CURRENT, sizeof(PZEM_CMD_CURRENT), 100);
	if(eStatus != HAL_OK) return eStatus;

	eStatus = HAL_UART_Receive(huart, rx_buf, 32, 200);
	if((eStatus == HAL_OK) && (rx_buf[1] == 0x04))
	{
		uint32_t val = ((uint32_t)rx_buf[3] << 24) | ((uint32_t)rx_buf[4] << 16)
		             | ((uint32_t)rx_buf[5] << 8) | rx_buf[6];
		*current = val * 0.001f;
	}
	return eStatus;
}

/* ==== Read Power ==== */
HAL_StatusTypeDef Pzem_ReadPower(float *power)
{
	HAL_StatusTypeDef eStatus;
	uint8_t rx_buf[32];

	eStatus = HAL_UART_Transmit(huart, (uint8_t*)PZEM_CMD_POWER, sizeof(PZEM_CMD_POWER), 100);
	if(eStatus != HAL_OK) return eStatus;

	eStatus = HAL_UART_Receive(huart, rx_buf, 32, 200);
	if((eStatus == HAL_OK) && (rx_buf[1] == 0x04))
	{
		uint32_t val = ((uint32_t)rx_buf[3] << 24) | ((uint32_t)rx_buf[4] << 16)
		             | ((uint32_t)rx_buf[5] << 8) | rx_buf[6];
		*power = val * 0.1f;
	}
	return eStatus;
}

/* ==== Read Energy ==== */
HAL_StatusTypeDef Pzem_ReadEnergy(float *energy)
{
	HAL_StatusTypeDef eStatus;
	uint8_t rx_buf[32];

	eStatus = HAL_UART_Transmit(huart, (uint8_t*)PZEM_CMD_ENERGY, sizeof(PZEM_CMD_ENERGY), 100);
	if(eStatus != HAL_OK) return eStatus;

	eStatus = HAL_UART_Receive(huart, rx_buf, 32, 200);
	if((eStatus == HAL_OK) && (rx_buf[1] == 0x04))
	{
		uint32_t val = ((uint32_t)rx_buf[3] << 24) | ((uint32_t)rx_buf[4] << 16)
		             | ((uint32_t)rx_buf[5] << 8) | rx_buf[6];
		*energy = val * 1.0f; // 1 LSB = 1 Wh
	}
	return eStatus;
}

/* ==== Read Frequency ==== */
HAL_StatusTypeDef Pzem_ReadFreq(float *freq)
{
	HAL_StatusTypeDef eStatus;
	uint8_t rx_buf[32];

	eStatus = HAL_UART_Transmit(huart, (uint8_t*)PZEM_CMD_FREQUENCY, sizeof(PZEM_CMD_FREQUENCY), 100);
	if(eStatus != HAL_OK) return eStatus;

	eStatus = HAL_UART_Receive(huart, rx_buf, 32, 200);
	if((eStatus == HAL_OK) && (rx_buf[1] == 0x04))
	{
		uint16_t val = (rx_buf[3] << 8) | rx_buf[4];
		*freq = val * 0.1f;
	}
	return eStatus;
}

/* ==== Read Power Factor ==== */
HAL_StatusTypeDef Pzem_ReadPowerFactor(float *pf)
{
	HAL_StatusTypeDef eStatus;
	uint8_t rx_buf[32];

	eStatus = HAL_UART_Transmit(huart, (uint8_t*)PZEM_CMD_POWER_FACTOR, sizeof(PZEM_CMD_POWER_FACTOR), 100);
	if(eStatus != HAL_OK) return eStatus;

	eStatus = HAL_UART_Receive(huart, rx_buf, 32, 200);
	if((eStatus == HAL_OK) && (rx_buf[1] == 0x04))
	{
		uint16_t val = (rx_buf[3] << 8) | rx_buf[4];
		*pf = val * 0.01f;
	}
	return eStatus;
}
