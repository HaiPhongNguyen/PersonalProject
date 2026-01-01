/*
 * Wiegand.h
 *
 *  Created on: Nov 24, 2025
 *      Author: Phong
 */

#ifndef INC_WIEGAND_H_
#define INC_WIEGAND_H_

#include "main.h"
#include <stdint.h>

/* ====== CONFIG: sửa lại theo chân bạn dùng trong CubeMX ====== */

// Ví dụ: D0 = PA8, D1 = PA9
#define WIEGAND_D0_GPIO_Port   MCU_D0_GPIO_Port
#define WIEGAND_D0_Pin         MCU_D0_Pin

#define WIEGAND_D1_GPIO_Port   MCU_D1_GPIO_Port
#define WIEGAND_D1_Pin         MCU_D1_Pin

// Thời gian xung Wiegand (có thể chỉnh theo đầu đọc)
#define WIEGAND_PULSE_US       80      // độ rộng xung 50–100 µs
#define WIEGAND_INTERVAL_US    2000    // khoảng cách giữa 2 bit ~1–2 ms

/* =============================================================== */

#ifdef __cplusplus
extern "C" {
#endif

void     Wiegand_Init(void);
void     delay_us(uint32_t us);                    // dùng DWT

void     Wiegand_SendBit(uint8_t bit);            // gửi 1 bit (0/1)
void     Wiegand_Send26_Raw(uint32_t data24);     // 24 bit data -> frame 26 bit chuẩn
void     Wiegand_Send26(uint8_t facility, uint16_t card);   // 26 bit chuẩn H10301
void     Wiegand_Send26_FromUID(uint8_t uid[4]);  // map UID[4] -> facility + card

void     Wiegand_Send34_Raw(uint32_t data32);      // 32 bit data -> frame 34 bit
void     Wiegand_Send34_FromUID(uint8_t uid[4]);   // dùng full UID 32 bit

#endif /* INC_WIEGAND_H_ */
