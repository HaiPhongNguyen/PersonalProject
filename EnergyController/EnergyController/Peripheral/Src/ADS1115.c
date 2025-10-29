/*
 * ADS1115.c
 *
 *  Created on: Jan 4, 2025
 *      Author: Phong
 */

#include "ADS1115.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

/*========================== Biến toàn cục ==========================*/
static I2C_HandleTypeDef *ADS1115_I2C_Handler;

extern SemaphoreHandle_t 	xI2c1Mutex;

static uint8_t  ADS1115_devAddress = 0x48;
static uint16_t ADS1115_dataRate   = ADS1115_CFG_DR_128;
static uint16_t ADS1115_pga        = ADS1115_CFG_PGA_TWO;
static float    ADS1115_voltCoef   = 0.0625f;

/*========================== Hàm khởi tạo ==========================*/
HAL_StatusTypeDef ADS1115_Init(I2C_HandleTypeDef *handler, uint8_t devAddr,
                               uint16_t dataRate, uint16_t pga)
{
    ADS1115_I2C_Handler = handler;
    ADS1115_devAddress = devAddr;
    ADS1115_dataRate   = dataRate;
    ADS1115_pga        = pga;

    // Tính hệ số điện áp
    switch (pga)
    {
        case ADS1115_CFG_PGA_TWOTHIRDS: ADS1115_voltCoef = 0.1875f; break;
        case ADS1115_CFG_PGA_ONE:       ADS1115_voltCoef = 0.125f;  break;
        case ADS1115_CFG_PGA_TWO:       ADS1115_voltCoef = 0.0625f; break;
        case ADS1115_CFG_PGA_FOUR:      ADS1115_voltCoef = 0.03125f; break;
        case ADS1115_CFG_PGA_EIGHT:     ADS1115_voltCoef = 0.015625f; break;
        case ADS1115_CFG_PGA_SIXTEEN:   ADS1115_voltCoef = 0.0078125f; break;
        default:                        ADS1115_voltCoef = 0.0625f; break;
    }
    return HAL_OK;
 
}

/*========================== Đọc 1 kênh single-ended ==========================*/
HAL_StatusTypeDef ADS1115_ReadSingleEnded(uint16_t muxPort, float *voltage)
{
    uint8_t config[2];
    uint8_t raw[2];
    uint16_t retry = 0;



    // Cấu hình ADC
    config[0] = ADS1115_OS | muxPort | ADS1115_pga | ADS1115_CFG_MODE;
    config[1] = ADS1115_dataRate | ADS1115_COMP_MODE | ADS1115_COMP_POL |
                ADS1115_COMP_LAT | ADS1115_COMP_QUEUE;

    // Ghi cấu hình
    if (HAL_I2C_Mem_Write(ADS1115_I2C_Handler, (ADS1115_devAddress << 1),
                          ADS1115_PTR_CFG_REG, 1, config, 2, ADS1115_TIMEOUT) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // Đợi ADC hoàn thành chuyển đổi
    while (1)
    {
        if (HAL_I2C_Mem_Read(ADS1115_I2C_Handler, (ADS1115_devAddress << 1),
                             ADS1115_PTR_CFG_REG, 1, config, 2, ADS1115_TIMEOUT) != HAL_OK)
        {
            return HAL_ERROR;
        }

        if (config[0] & ADS1115_OS) break;  // Bit OS = 1 => hoàn tất

        osDelay(5);
        if (++retry > 100)  // timeout ~500ms
        {
            return HAL_TIMEOUT;
        }
    }

    // Đọc dữ liệu ADC
    if (HAL_I2C_Mem_Read(ADS1115_I2C_Handler, (ADS1115_devAddress << 1),
                         ADS1115_PTR_COV_REG, 1, raw, 2, ADS1115_TIMEOUT) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // Tính điện áp
    int16_t rawVal = (int16_t)((raw[0] << 8) | raw[1]);
    *voltage = (rawVal * ADS1115_voltCoef) / 1000.0f; // mV → V

    return HAL_OK;
}

/*========================== Đọc liên tục ==========================*/
HAL_StatusTypeDef ADS1115_ReadContinuous(float *voltage)
{
    uint8_t raw[2];

    if (HAL_I2C_Mem_Read(ADS1115_I2C_Handler, (ADS1115_devAddress << 1),
                         ADS1115_PTR_COV_REG, 1, raw, 2, ADS1115_TIMEOUT) != HAL_OK)
    {
        return HAL_ERROR;
    }

    int16_t rawVal = (int16_t)((raw[0] << 8) | raw[1]);
    *voltage = (rawVal * ADS1115_voltCoef) / 1000.0f;

    return HAL_OK;
}

