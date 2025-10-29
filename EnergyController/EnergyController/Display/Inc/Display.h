/*
 * Display.h
 *
 *  Created on: Oct 18, 2025
 *      Author: Phong
 */

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_

#include "LCD2004.h"
#include "Read_Sensor.h"
#include <stdint.h>
#include "DS1307.h"
/* ==== Biểu tượng (định nghĩa ở Display.c) ==== */
extern uint8_t sun[8];
extern uint8_t wind_turbine[8];
extern uint8_t battery[8];
extern DateTime_t g_endTime[5];
extern uint8_t    g_endTime_set[5];
/* ==== Prototype ==== */

/**
 * @brief Khởi tạo LCD và các ký tự CGRAM (gọi 1 lần sau khi I2C sẵn sàng)
 * @param hi2c: handler I2C (vd: &hi2c1)
 * @param lcd_addr: địa chỉ 7-bit của module I2C (vd: 0x27)
 */
void Display_Init(I2C_HandleTypeDef *hi2c, uint8_t lcd_addr);

/**
 * @brief Hiển thị page 1 (tổng quan năng lượng) từ SensorData_t
 * @param data: con trỏ tới struct đọc từ Sensor_ReadAll()
 */
void Display_UpdatePage1(const SensorData_t *data, const DateTime_t *timeStr);

/**
 * @brief Hiển thị page 2: Select Port
 */
void Display_Page2(void);

/**
 * @brief Hiển thị page 3: Time Charging menu
 */
void Display_Page3(void);

/**
 * @brief Hiển thị page 4: Enter time
 */
void Display_Page4(void);
void Display_Page4_HandleKey(uint8_t key);
uint16_t Display_GetInputMinutes(void);
void Display_ClearInputBuffer(void);
/**
 * @brief Hiển thị page 5: info của một cổng (port)
 * @param port: số cổng (1..5)
 * @param voltage: V
 * @param current: A
 * @param endTime: chuỗi "HH:MM"
 */
void Display_Page5(uint8_t port, float voltage, float current, const DateTime_t *endTime);

/**
 * @brief Hiển thị page5 clone (chọn port 1..5)
 */
void Display_Page5_Select(uint8_t portSelected, const SourceOutputData_t *OutputSource);

// Display.h
void Display_Page5_1(uint8_t timeOption);

#endif /* INC_DISPLAY_H_ */
