/*
 * Display.c
 *
 *  Created on: Oct 18, 2025
 *      Author: Phong
 *
 *  Sử dụng lcd2004.h API của bạn:
 *   - lcd_init(I2C_HandleTypeDef *hi2c, uint8_t addr);
 *   - lcd_createChar(location, map);
 *   - lcd_writeCustom(charCode);
 *   - lcd_clear(), lcd_setCursor(col,row), lcd_print(fmt,...)
 *
 *  Không dùng HAL_Delay, dùng vTaskDelay (FreeRTOS).
 */

#include "Display.h"
#include "LCD2004.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include "math.h"
#include "stdlib.h"

/* ======================== Biến toàn cục nội bộ ======================== */
static char inputBuffer[8] = {0};   // lưu chuỗi nhập phút (tối đa 4 ký tự)
static uint8_t inputIndex = 0;      // chỉ số hiện tại khi nhập

/* ==== Biểu tượng CGRAM (định nghĩa extern ở header) ==== */
uint8_t sun[8] = {
  0b00100,
  0b10101,
  0b01110,
  0b11111,
  0b01110,
  0b10101,
  0b00100,
  0b00000
};

uint8_t wind_turbine[8] = {
  0b00100,
  0b01100,
  0b11111,
  0b01100,
  0b00100,
  0b00100,
  0b00100,
  0b00000
};

uint8_t battery[8] = {
  0b01110,
  0b11111,
  0b10001,
  0b10001,
  0b10001,
  0b11111,
  0b01110,
  0b00000
};

/* End time theo từng cổng (1..5) */
DateTime_t g_endTime[5];
uint8_t    g_endTime_set[5] = {0};
/* ==== Nội bộ: địa chỉ lcd đã nạp (chỉ để biết đã init hay chưa) ==== */
static uint8_t g_lcd_inited = 0;

/* ==== Hàm khởi tạo LCD và nạp CGRAM ==== */
void Display_Init(I2C_HandleTypeDef *hi2c, uint8_t lcd_addr)
{
    if (hi2c == NULL) return;

    /* Gọi hàm khởi tạo thư viện của bạn */
    lcd_init(hi2c, lcd_addr);

    /* Nạp các ký tự custom vào CGRAM (0..2) */
    lcd_createChar(0, sun);          // mã 0
    lcd_createChar(1, wind_turbine); // mã 1
    lcd_createChar(2, battery);      // mã 2

    /* Hiển thị thông báo khởi động ngắn (dùng vTaskDelay để không block HAL) */
    lcd_clear();
    lcd_setCursor(0, 0);
    lcd_print(" Smart Energy Sys ");
    lcd_setCursor(0, 1);
    lcd_print("   Initializing   ");
    osDelay(2000U);
    lcd_clear();

    g_lcd_inited = 1;
}

/* ==== Page1: hiển thị dữ liệu từ SensorData_t ==== */
/* timeStr: "HH:MM:SS" hoặc NULL; soc_percent: 0..100 or 255 if not used */
static void lcd_print_padded_20(const char *s)
{
    lcd_print(s);
    size_t n = strlen(s);
    while (n++ < 20)
        lcd_print(" ");   // ✅ truyền chuỗi " " thay vì ký tự ' '
}

void Display_UpdatePage1(const SensorData_t *data, const DateTime_t *timeStr)
{
    if (!g_lcd_inited) return;
    if (data == NULL)   return;

    char buf[26];               // 26 ký tự + null
    int  soc_percent = 100;

    /* Dòng 1: Solar - icon + U I */
    lcd_setCursor(0, 0);
    lcd_writeCustom(0);
    /* " U:xx.xV I:x.xxA" -> tổng không quá 19 ký tự còn lại */
    snprintf(buf, sizeof(buf), " U:%5.1fV I:%4.2fA",
             (double)data->solarVoltage, (double)data->solarCurrent);
    lcd_print_padded_20(buf);

    /* Dòng 2: Wind */
    lcd_setCursor(0, 1);
    lcd_writeCustom(1);
    snprintf(buf, sizeof(buf), " U:%5.1fV I:%4.2fA",
             (double)data->turbinVoltage, (double)data->turbinCurrent);
    lcd_print_padded_20(buf);

    /* Dòng 3: Battery */
    lcd_setCursor(0, 2);
    lcd_writeCustom(2);

    /* Tính SOC (đơn giản theo tỉ lệ V/13.2V), rồi clamp 0..100 để tránh in rác */
    float ratio = data->batteryVoltage / 13.2f;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    soc_percent = (int)lroundf(ratio * 100.0f);

    /* Nếu muốn, bạn có thể dùng mô hình SOC thực tế hơn; tạm thời giữ cách tính đơn giản */
    snprintf(buf, sizeof(buf), " U:%5.2fV SOC:%3d",
             (double)data->batteryVoltage, soc_percent);
    lcd_print_padded_20(buf);

    /* Dòng 4: Hiển thị thời gian (HH:MM:SS DD/MM/YY) */
    lcd_setCursor(0, 3);

    if (timeStr != NULL)
    {
        /* Độ dài: " HH:MM:SS DD/MM/YY" = 1 + 8 + 1 + 8 = 18 <= 20 (an toàn) */
        snprintf(buf, sizeof(buf), " %02u:%02u:%02u %02u/%02u/%02u",
                 (uint8_t)timeStr->hour,
                 (uint8_t)timeStr->min,
                 (uint8_t)timeStr->sec,
                 (uint8_t)timeStr->day,
                 (uint8_t)timeStr->month,
                 (uint8_t)timeStr->year);  // year: 2 số cuối
    }
    else
    {
        /* Dự phòng khi chưa có thời gian */
        snprintf(buf, sizeof(buf), " --:--:-- --/--/--");
    }
    lcd_print_padded_20(buf);
}

/* ==== Page2: Select Port (giữ đúng format, dấu '.' trong prompt thay bằng khoảng trắng) ==== */
void Display_Page2(void)
{
    if (!g_lcd_inited) return;
    lcd_clear();

    /* Dòng 1: tiêu đề căn giữa */
    lcd_setCursor(5, 0);
    lcd_print("Select Port");

    /* Dòng 2-4 theo mô tả, dùng khoảng trắng thay dấu '.' và căn thủ công */
    lcd_setCursor(0, 1);
    lcd_print("1.Port 1  5.AC Port");
    lcd_setCursor(0, 2);
    lcd_print("2.Port 2  3.Port 3");
    lcd_setCursor(0, 3);
    lcd_print("4.Port 4");
}

/* ==== Page3: Time Charging menu ==== */
void Display_Page3(void)
{
    if (!g_lcd_inited) return;
    lcd_clear();
    lcd_setCursor(2, 0);
    lcd_print("Time Charging");
    lcd_setCursor(0, 1);
    lcd_print("1. 30min  2. 1 hour");
    lcd_setCursor(0, 2);
    lcd_print("3. 2 hour 4. 4 hour");
    lcd_setCursor(0, 3);
    lcd_print("5. Custom");
}

/* ==== Page4: Enter time ==== */
void Display_Page4(void)
{
    lcd_clear();
    lcd_setCursor(2, 0);
    lcd_print("Enter charge time");

    lcd_setCursor(5, 1);
    lcd_print(".. min ");

    lcd_setCursor(0, 3);
    lcd_print("Press # to confirm");

    memset(inputBuffer, 0, sizeof(inputBuffer));
    inputIndex = 0;
}

uint16_t Display_GetInputMinutes(void)
{
    return atoi(inputBuffer);
}

void Display_ClearInputBuffer(void)
{
    memset(inputBuffer, 0, sizeof(inputBuffer));
    inputIndex = 0;
    lcd_setCursor(5, 1);
    lcd_print("    min");
}

/* ==== Xử lý phím cho Page 4 ==== */
void Display_Page4_HandleKey(uint8_t key)
{
    if (key >= '0' && key <= '9')
    {
        if (inputIndex < 4)
        {
            inputBuffer[inputIndex++] = key;
            inputBuffer[inputIndex] = '\0';

            // Cập nhật LCD ngay khi gõ
            lcd_setCursor(5, 1);
            lcd_print("    "); // Xóa cũ
            lcd_setCursor(5, 1);
            lcd_print(inputBuffer);
            lcd_print(" min ");
        }
    }
}


/* ==== Page5: Charging info for a port ==== */
void Display_Page5(uint8_t port, float voltage, float current, const DateTime_t *endTime)
{
    if (!g_lcd_inited) return;

    char buf[26];  // rộng hơn 20 để tránh cảnh báo tràn khi snprintf tạm tính

    lcd_clear();

    /* Dòng 1: tiêu đề căn giữa */
    int len = snprintf(buf, sizeof(buf), "Charging Port %u", (unsigned)port);
    if (len < 0) len = 0;
    if (len > 20) buf[20] = '\0', len = 20;  // cắt nếu quá dài
    int x = (20 - len) / 2;
    if (x < 0) x = 0;
    lcd_setCursor((uint8_t)x, 0);
    lcd_print_padded_20(buf);

    /* Dòng 2: Voltage */
    lcd_setCursor(0, 1);
    // Ví dụ: "Voltage: 12.34 V" (<= 20 ký tự, sau đó được đệm)
    snprintf(buf, sizeof(buf), "Voltage: %.2f V", (double)voltage);
    lcd_print_padded_20(buf);

    /* Dòng 3: Current */
    lcd_setCursor(0, 2);
    // Ví dụ: "Current: 1.23 A"
    snprintf(buf, sizeof(buf), "Current: %.2f A", (double)current);
    lcd_print_padded_20(buf);

    /* Dòng 4: End time */
    lcd_setCursor(0, 3);
    if (endTime) {
        // Định dạng gọn để vừa 20 ký tự: "End: HH:MM:SS DD/MM"
        // 5 + 8 + 1 + 5 = 19 ký tự
        snprintf(buf, sizeof(buf), "End: %02u:%02u:%02u %02u/%02u",
                 (unsigned)endTime->hour,
                 (unsigned)endTime->min,
                 (unsigned)endTime->sec,
                 (unsigned)endTime->day,
                 (unsigned)endTime->month);
    } else {
        snprintf(buf, sizeof(buf), "End: --:--:-- --/--");
    }
    lcd_print_padded_20(buf);
}

/* ==== Page5_Select ==== */
void Display_Page5_Select(uint8_t portSelected, const SourceOutputData_t *OutputSource)
{
    /* Placeholder voltage/current demo */
	float voltage[5] = {
        OutputSource->port1Voltage,
        OutputSource->port2Voltage,
        OutputSource->port3Voltage,
        OutputSource->port4Voltage,
        OutputSource->IvVoltage
    };

    float current[5] = {
        OutputSource->port1Current,
        OutputSource->port2Current,
        OutputSource->port3Current,
        OutputSource->port4Current,
        OutputSource->IvCurrent
    };

    if (portSelected >= 1 && portSelected <= 5)
    {
        uint8_t idx = (uint8_t)(portSelected - 1);
        const DateTime_t *et = g_endTime_set[idx] ? &g_endTime[idx] : NULL; // dùng endTime đã tính
        Display_Page5(portSelected, voltage[idx], current[idx], et);
    }
    else
    {
        lcd_clear();
        lcd_setCursor(3, 1);
        lcd_print("Invalid Port!");
        osDelay(800);
    }
}


void Display_Page5_1(uint8_t timeOption)
{
    if (!g_lcd_inited) return;
    lcd_clear();

    lcd_setCursor(3, 0);
    lcd_print("Selected Time");

    lcd_setCursor(0, 1);
    switch (timeOption) {
        case 1: lcd_print("1) 30 minutes"); break;
        case 2: lcd_print("2) 1 hour");     break;
        case 3: lcd_print("3) 2 hours");    break;
        case 4: lcd_print("4) 4 hours");    break;
        case 5: lcd_print("5) Custom");     break;
        default: lcd_print("Invalid");      break;
    }

    lcd_setCursor(0, 3);
    lcd_print("Press any key...");
}

