/*
 * ADS1115.h
 *
 *  Created on: Oct 12, 2025
 *      Author: Phong
 */

#ifndef INC_ADS1115_H_
#define INC_ADS1115_H_

#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "main.h"
#ifdef __cplusplus
extern "C" {
#endif

/*=====================================================================
 * ADS1115 I2C ADDRESS
 *====================================================================*/
#define ADS1115_ADDR_DF     0x48
#define ADS1115_ADDR_GND    (ADS1115_ADDR_DF)
#define ADS1115_ADDR_VDD    (ADS1115_ADDR_DF + 1)
#define ADS1115_ADDR_SDA    (ADS1115_ADDR_DF + 2)
#define ADS1115_ADDR_SCL    (ADS1115_ADDR_DF + 3)

/*=====================================================================
 * REGISTER MAP
 *====================================================================*/
#define ADS1115_PTR_COV_REG     0x00
#define ADS1115_PTR_CFG_REG     0x01
#define ADS1115_PTR_LOW_REG     0x02
#define ADS1115_PTR_HIG_REG     0x03

/*=====================================================================
 * CONFIG REGISTER (MSB)
 *====================================================================*/
#define ADS1115_OS                  (0b1 << 7)  // Start single conversion

#define ADS1115_CFG_AIN0            (0b100 << 4)
#define ADS1115_CFG_AIN1            (0b101 << 4)
#define ADS1115_CFG_AIN2            (0b110 << 4)
#define ADS1115_CFG_AIN3            (0b111 << 4)

/* Gain Amplifier Config (PGA) */
#define ADS1115_CFG_PGA_TWOTHIRDS   (0b000 << 1)   // ±6.144V
#define ADS1115_CFG_PGA_ONE         (0b001 << 1)   // ±4.096V
#define ADS1115_CFG_PGA_TWO         (0b010 << 1)   // ±2.048V
#define ADS1115_CFG_PGA_FOUR        (0b011 << 1)   // ±1.024V
#define ADS1115_CFG_PGA_EIGHT       (0b100 << 1)   // ±0.512V
#define ADS1115_CFG_PGA_SIXTEEN     (0b111 << 1)   // ±0.256V

#define ADS1115_CFG_MODE            (0b1)          // Single-shot mode

/*=====================================================================
 * CONFIG REGISTER (LSB)
 *====================================================================*/
#define ADS1115_CFG_DR_8            (0b000 << 5)
#define ADS1115_CFG_DR_16           (0b001 << 5)
#define ADS1115_CFG_DR_32           (0b010 << 5)
#define ADS1115_CFG_DR_64           (0b011 << 5)
#define ADS1115_CFG_DR_128          (0b100 << 5)
#define ADS1115_CFG_DR_250          (0b101 << 5)
#define ADS1115_CFG_DR_475          (0b110 << 5)
#define ADS1115_CFG_DR_860          (0b111 << 5)

#define ADS1115_COMP_MODE           (0b0 << 4)
#define ADS1115_COMP_POL            (0b0 << 3)
#define ADS1115_COMP_LAT            (0b0 << 2)
#define ADS1115_COMP_QUEUE          (0b11)

/*=====================================================================
 * TIMEOUTS
 *====================================================================*/
#define ADS1115_TIMEOUT             50   // I2C timeout (ms)

/*=====================================================================
 * CONFIG OPTIONS
 *====================================================================*/

/*=====================================================================
 * FUNCTION PROTOTYPES
 *====================================================================*/
HAL_StatusTypeDef ADS1115_Init(I2C_HandleTypeDef *handler, uint8_t devAddr,
                               uint16_t dataRate, uint16_t pga);

HAL_StatusTypeDef ADS1115_ReadSingleEnded(uint16_t muxPort, float *voltage);

HAL_StatusTypeDef ADS1115_ReadContinuous(float *voltage);

#ifdef __cplusplus
}
#endif

#endif /* INC_ADS1115_H_ */
