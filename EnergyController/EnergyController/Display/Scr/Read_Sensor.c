/*
 * read_sensor.c
 *  Created on: Oct 18, 2025
 *      Author: Phong
 *
 * Mapping ADS1115:
 *  - AIN0 -> solar panel voltage (node after divider) -> convert to Vpanel
 *  - AIN1 -> turbin voltage (node after divider)  -> convert to Vturbine
 *  - AIN2 -> turbin current (ACS712 30A) -> Vout, convert to A using sensitivity
 *  - AIN3 -> solar current (ACS712 30A)  -> Vout, convert to A
 * ADC1 CH0 (PA0) -> battery voltage via divider R3/R4: ADC_Read() trả ra Vbattery
 */


#include "ADS1115.h"
#include "ADC.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>
#include "Read_Sensor.h"

static INA219_HandleTypeDef ina219_port1;
static INA219_HandleTypeDef ina219_port2;
static INA219_HandleTypeDef ina219_port3;
static INA219_HandleTypeDef ina219_port4;

static UART_HandleTypeDef *pzem_uart_handle = NULL;

/* ---------- Config (thay giá trị thực tế khi có) ---------- */
/* Divider R1-R2 for ADS AIN0 (solar): Vs = Vnode * (R1 + R2) / R2 */
#define R1_SOLAR_OHM   100.0f   /* R1 nối nguồn */
#define R2_SOLAR_OHM   33.0f    /* R2 nối đất */

/* Divider R1-R2 for ADS AIN1 (turbine) */
#define R1_TURBINE_OHM 100.0f
#define R2_TURBINE_OHM 33.0f

/* ACS712-30A sensitivity (V per A) */
#define ACS712_30A_SENS 0.066f   /* 66 mV/A typical */

/* Nếu ACS được cấp Vcc khác (vd 5V) và ADS1115 dùng 3.3V, bạn phải chú ý tỉ lệ.
   Mặc định ta giả sử ADS1115 và ACS dùng cùng Vref hợp lý. */

/* ADS1115 MUX constants: dùng những macro đã khai báo trong ADS1115.h */
#define ADS_AIN0 ADS1115_CFG_AIN0
#define ADS_AIN1 ADS1115_CFG_AIN1
#define ADS_AIN2 ADS1115_CFG_AIN2
#define ADS_AIN3 ADS1115_CFG_AIN3

/* ---------- Biến nội bộ ---------- */
static I2C_HandleTypeDef *ads_i2c = NULL;
static ADC_HandleTypeDef *adc_in  = NULL;

/* Offsets (điện áp zero của ACS measured Vout at 0A) */
typedef struct {
    float solarCurrentVzero;   /* Vout của ACS đo solar current khi I=0 */
    float turbinCurrentVzero;  /* Vout của ACS đo turbin current khi I=0 */
} SensorCalib_t;

static SensorCalib_t calib = {
    .solarCurrentVzero = 2.5,
    .turbinCurrentVzero = 2.5,
};

/* ---------- Khởi tạo ---------- */
void Sensor_Init(I2C_HandleTypeDef *hi2c_ads, ADC_HandleTypeDef *hadc_in)
{
    ads_i2c = hi2c_ads;
    adc_in  = hadc_in;

    /* Khởi tạo ADS1115 và ADC - dùng config mặc định, bạn có thể thay addr/pga/dr */
    ADS1115_Init(ads_i2c, ADS1115_ADDR_GND, ADS1115_CFG_DR_128, ADS1115_CFG_PGA_TWO);
    ADC_Init(adc_in);
}

/* ---------- Calibrate: lấy trung bình sampleCount mẫu cho mỗi ACS ---------- */
void Sensor_Calibrate(uint16_t sampleCount)
{
    if (sampleCount == 0) sampleCount = 1;

    float sumSolarV = 0.0f;
    float sumTurbinV = 0.0f;
    float tmp = 0.0f;

    for (uint16_t i = 0; i < sampleCount; i++)
    {
        /* Đọc AIN3 => solar current sensor output (per mapping) */
        if (ADS1115_ReadSingleEnded(ADS_AIN3, &tmp) == HAL_OK)
            sumSolarV += tmp;

        /* Đọc AIN2 => turbin current sensor output */
        if (ADS1115_ReadSingleEnded(ADS_AIN2, &tmp) == HAL_OK)
            sumTurbinV += tmp;

        osDelay(10);
    }

    calib.solarCurrentVzero  = sumSolarV / (float)sampleCount;
    calib.turbinCurrentVzero = sumTurbinV / (float)sampleCount;
}

/* ---------- Đọc tất cả cảm biến (public) ---------- */
void Sensor_ReadAll(SensorData_t *data)
{
    if (data == NULL) return;

    float vnode = 0.0f;

    /* --- AIN0: solar panel voltage node --- */
    if (ADS1115_ReadSingleEnded(ADS_AIN0, &vnode) == HAL_OK)
    {
        /* convert node voltage -> source voltage */
        float gain = (R1_SOLAR_OHM + R2_SOLAR_OHM) / R2_SOLAR_OHM;
        data->solarVoltage = vnode * gain;
    }
    else
    {
        data->solarVoltage = 0.0f;
    }

    /* --- AIN1: turbine voltage node --- */
    if (ADS1115_ReadSingleEnded(ADS_AIN1, &vnode) == HAL_OK)
    {
        float gain = (R1_TURBINE_OHM + R2_TURBINE_OHM) / R2_TURBINE_OHM;
        data->turbinVoltage = vnode * gain;
    }
    else
    {
        data->turbinVoltage = 0.0f;
    }

    /* --- AIN2: turbine current (ACS712 30A) --- */
    if (ADS1115_ReadSingleEnded(ADS_AIN2, &vnode) == HAL_OK)
    {
        /* subtract zero offset measured in calibrate, then divide by sensitivity */
        data->turbinCurrent = (vnode - calib.turbinCurrentVzero) / ACS712_30A_SENS;
    }
    else
    {
        data->turbinCurrent = 0.0f;
    }

    /* --- AIN3: solar current (ACS712 30A) --- */
    if (ADS1115_ReadSingleEnded(ADS_AIN3, &vnode) == HAL_OK)
    {
        data->solarCurrent = (vnode - calib.solarCurrentVzero) / ACS712_30A_SENS;
    }
    else
    {
        data->solarCurrent = 0.0f;
    }

    /* --- ADC internal channel (PA0): battery voltage via divider R3/R4 ---
       Note: your ADC_Read() implementation already returns the scaled battery voltage
    */
    float vbatt = 0.0f;
    if (ADC_Read(&vbatt) == HAL_OK)
    {
        data->batteryVoltage = vbatt;
    }
    else
    {
        data->batteryVoltage = 0.0f;
    }
}

/**
 * @brief Khởi tạo các cảm biến INA219 và PZEM004T
 * @param hi2c  - I2C handle dùng chung cho INA219
 * @param huart - UART handle của PZEM004T
 */
void SourceOutput_Init(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart)
{
    // ====== INA219 port1 ======
    ina219_port1.hi2c = hi2c;
    ina219_port1.address = 0x40;   // A1A0 = 00
    INA219_Init(&ina219_port1);
    INA219_SetCalibration(&ina219_port1, 0.1f, 3.2f); // Rshunt=0.1Ω, Imax=3.2A (tùy bạn chỉnh)

    // ====== INA219 port2 ======
    ina219_port2.hi2c = hi2c;
    ina219_port2.address = 0x41;   // A1A0 = 01
    INA219_Init(&ina219_port2);
    INA219_SetCalibration(&ina219_port2, 0.1f, 3.2f);

    // ====== INA219 port3 ======
    ina219_port3.hi2c = hi2c;
    ina219_port3.address = 0x42;   // A1A0 = 10
    INA219_Init(&ina219_port3);
    INA219_SetCalibration(&ina219_port3, 0.1f, 3.2f);

    // ====== INA219 port4 ======
    ina219_port4.hi2c = hi2c;
    ina219_port4.address = 0x43;   // A1A0 = 11
    INA219_Init(&ina219_port4);
    INA219_SetCalibration(&ina219_port4, 0.1f, 3.2f);

    // ====== PZEM ======
    pzem_uart_handle = huart;
    Pzem_Init(pzem_uart_handle);
}

/**
 * @brief Đọc toàn bộ dữ liệu điện áp và dòng của 4 port và Iv từ PZEM004T
 * @param data - struct lưu kết quả
 */
void SourceOutput_ReadAll(SourceOutputData_t *data)
{
    if (data == NULL) return;

    // === Đọc từ 4 INA219 ===
    data->port1Voltage = INA219_ReadBusVoltage(&ina219_port1);
    data->port1Current = INA219_ReadCurrent(&ina219_port1);

    data->port2Voltage = INA219_ReadBusVoltage(&ina219_port2);
    data->port2Current = INA219_ReadCurrent(&ina219_port2);

    data->port3Voltage = INA219_ReadBusVoltage(&ina219_port3);
    data->port3Current = INA219_ReadCurrent(&ina219_port3);

    data->port4Voltage = INA219_ReadBusVoltage(&ina219_port4);
    data->port4Current = INA219_ReadCurrent(&ina219_port4);

    // === Đọc từ PZEM004T ===
    float v = 0, i = 0;
    if (Pzem_ReadVoltage(&v) == HAL_OK)
        data->IvVoltage = v;
    else
        data->IvVoltage = 0;

    if (Pzem_ReadCurrent(&i) == HAL_OK)
        data->IvCurrent = i;
    else
        data->IvCurrent = 0;
}
