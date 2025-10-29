/*
 * lcd2004.h
 * LCD 20x4 I2C library for STM32 HAL + FreeRTOS (Arduino-style)
 * Author: Phong
 */

#ifndef INC_LCD2004_H_
#define INC_LCD2004_H_

#include "stm32f1xx_hal.h"
#include <string.h>


/* ====== Cấu hình ====== */
#include "cmsis_os.h"


/* Command definitions */
#define LCD_CLEAR_DISPLAY        0x01
#define LCD_RETURN_HOME          0x02
#define LCD_ENTRY_MODE_SET       0x04
#define LCD_DISPLAY_CONTROL      0x08
#define LCD_CURSOR_SHIFT         0x10
#define LCD_FUNCTION_SET         0x20
#define LCD_SET_CGRAM_ADDR       0x40
#define LCD_SET_DDRAM_ADDR       0x80

#define LCD_ENTRY_LEFT           0x02
#define LCD_ENTRY_SHIFT_DECREMENT 0x00

#define LCD_DISPLAY_ON           0x04
#define LCD_CURSOR_ON            0x02
#define LCD_BLINK_ON             0x01

#define LCD_4BIT_MODE            0x00
#define LCD_2LINE                0x08
#define LCD_5x8DOTS              0x00

#define LCD_BACKLIGHT            0x08
#define LCD_NOBACKLIGHT          0x00

#define En 0x04
#define Rw 0x02
#define Rs 0x01

typedef struct {
	I2C_HandleTypeDef *hi2c;
	uint8_t address;
	uint8_t display_control;
	uint8_t display_function;
	uint8_t display_mode;
	uint8_t backlight;
} LCD2004_HandleTypeDef;

extern LCD2004_HandleTypeDef lcd;

/* Arduino-style APIs */
void lcd_init(I2C_HandleTypeDef *hi2c, uint8_t addr);
void lcd_clear(void);
void lcd_home(void);
void lcd_setCursor(uint8_t col, uint8_t row);
void lcd_print(const char *fmt, ...);
void lcd_display(void);
void lcd_noDisplay(void);
void lcd_backlight(void);
void lcd_noBacklight(void);
void lcd_writeCustom(uint8_t charCode);
void lcd_createChar(uint8_t location, uint8_t charmap[]);

#endif /* INC_LCD2004_H_ */
