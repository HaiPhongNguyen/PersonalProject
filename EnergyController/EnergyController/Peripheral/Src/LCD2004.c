/*
 * lcd2004.c
 * LCD 20x4 I2C library for STM32 HAL + FreeRTOS
 * Author: Phong
 */

#include "lcd2004.h"
#include "stdarg.h"
#include "stdio.h"
LCD2004_HandleTypeDef lcd;

/* ==== Private prototypes ==== */
static void lcd_send(uint8_t value, uint8_t mode);
static void lcd_write4bits(uint8_t value);
static void lcd_pulseEnable(uint8_t data);
static void lcd_command(uint8_t value);
static void lcd_printString(const char *str);
void lcd_init(I2C_HandleTypeDef *hi2c, uint8_t addr)
{
	lcd.hi2c = hi2c;
	lcd.address = addr << 1;
	lcd.backlight = LCD_BACKLIGHT;
	lcd.display_function = LCD_4BIT_MODE | LCD_2LINE | LCD_5x8DOTS;

	osDelay(50);

	lcd_write4bits(0x03 << 4);
	osDelay(5);
	lcd_write4bits(0x03 << 4);
	osDelay(5);
	lcd_write4bits(0x03 << 4);
	osDelay(1);
	lcd_write4bits(0x02 << 4); // 4-bit mode

	lcd_command(LCD_FUNCTION_SET | lcd.display_function);
	lcd.display_control = LCD_DISPLAY_ON;
	lcd_display();
	lcd_clear();
	lcd.display_mode = LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DECREMENT;
	lcd_command(LCD_ENTRY_MODE_SET | lcd.display_mode);
}

void lcd_clear(void)
{
	lcd_command(LCD_CLEAR_DISPLAY);
	osDelay(2);
}

void lcd_home(void)
{
	lcd_command(LCD_RETURN_HOME);
	osDelay(2);
}

void lcd_setCursor(uint8_t col, uint8_t row)
{
	const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
	if (row > 3) row = 3;
	lcd_command(LCD_SET_DDRAM_ADDR | (col + row_offsets[row]));
}

void lcd_print(const char *fmt, ...)
{
    char buf[64];
    memset(buf, 0, sizeof(buf));
    va_list args;
    va_start(args, fmt);                // lấy danh sách các tham số sau fmt
    vsnprintf(buf, sizeof(buf), fmt, args); // giống sprintf() nhưng an toàn hơn
    va_end(args);

    lcd_printString(buf);           // in chuỗi ra LCD
}

void lcd_display(void)
{
	lcd.display_control |= LCD_DISPLAY_ON;
	lcd_command(LCD_DISPLAY_CONTROL | lcd.display_control);
}

void lcd_noDisplay(void)
{
	lcd.display_control &= ~LCD_DISPLAY_ON;
	lcd_command(LCD_DISPLAY_CONTROL | lcd.display_control);
}

void lcd_backlight(void)
{
	lcd.backlight = LCD_BACKLIGHT;
	HAL_I2C_Master_Transmit(lcd.hi2c, lcd.address, &lcd.backlight, 1, 10);
}

void lcd_noBacklight(void)
{
	lcd.backlight = LCD_NOBACKLIGHT;
	HAL_I2C_Master_Transmit(lcd.hi2c, lcd.address, &lcd.backlight, 1, 10);
}

/* ==== Private functions ==== */
static void lcd_send(uint8_t value, uint8_t mode)
{
	uint8_t high = value & 0xF0;
	uint8_t low  = (value << 4) & 0xF0;
	lcd_write4bits(high | mode);
	lcd_write4bits(low | mode);
}

static void lcd_write4bits(uint8_t value)
{
	uint8_t data = value | lcd.backlight;
	lcd_pulseEnable(data);
}

static void lcd_pulseEnable(uint8_t data)
{
	HAL_I2C_Master_Transmit(lcd.hi2c, lcd.address, &data, 1, 10);
	osDelay(1);

	uint8_t pulse = data | En;
	HAL_I2C_Master_Transmit(lcd.hi2c, lcd.address, &pulse, 1, 10);
	osDelay(1);

	pulse = data & ~En;
	HAL_I2C_Master_Transmit(lcd.hi2c, lcd.address, &pulse, 1, 10);
	osDelay(1);
}

static void lcd_command(uint8_t value)
{
	lcd_send(value, 0);
}

static void lcd_printString(const char *str)
{
    while (*str)
    {
    	lcd_send(*str++, Rs);
    }
}

void lcd_writeCustom(uint8_t charCode)
{
    lcd_send(charCode, Rs);
}

void lcd_createChar(uint8_t location, uint8_t charmap[])
{
    location &= 0x7; // chỉ cho phép 0–7 (8 ký tự custom)
    lcd_command(LCD_SET_CGRAM_ADDR | (location << 3));
    for (int i = 0; i < 8; i++)
    {
        lcd_send(charmap[i], Rs);
    }
}
