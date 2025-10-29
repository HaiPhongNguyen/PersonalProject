#include "INA219.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

extern SemaphoreHandle_t 	xI2c2Mutex;

/* ==== Internal helper functions ==== */
static HAL_StatusTypeDef INA219_WriteRegister(INA219_HandleTypeDef *ina, uint8_t reg, uint16_t value)
{	
	HAL_StatusTypeDef eStatus;
	uint8_t data[3];
	data[0] = reg;
	data[1] = (value >> 8) & 0xFF;
	data[2] = value & 0xFF;

	eStatus = HAL_I2C_Master_Transmit(ina->hi2c, ina->address << 1, data, 3, pdMS_TO_TICKS(100U));

	return eStatus;
}

static HAL_StatusTypeDef INA219_ReadRegister(INA219_HandleTypeDef *ina, uint8_t reg, uint16_t *rxData)
{
    HAL_StatusTypeDef eStatus;
    uint8_t data[2];

    // Gửi địa chỉ thanh ghi cần đọc
    eStatus = HAL_I2C_Master_Transmit(ina->hi2c, ina->address << 1, &reg, 1, pdMS_TO_TICKS(100));
    if (eStatus != HAL_OK)
    {
        return eStatus;
    }

    // Đọc 2 byte dữ liệu
    eStatus = HAL_I2C_Master_Receive(ina->hi2c, ina->address << 1, data, 2, pdMS_TO_TICKS(100));

    if (eStatus == HAL_OK)
    {
        *rxData = (data[0] << 8) | data[1];
    }

    return eStatus;
}


/* ==== Public API ==== */
HAL_StatusTypeDef INA219_Reset(INA219_HandleTypeDef *ina)
{	
	HAL_StatusTypeDef eStatus;
	eStatus = INA219_WriteRegister(ina, INA_CFG_REG, INA_CFG_RST_BIT);
	return eStatus;
}

HAL_StatusTypeDef INA219_Init(INA219_HandleTypeDef *ina)
{	
	HAL_StatusTypeDef eStatus = HAL_OK;
    
	// Reset device
	eStatus |= INA219_Reset(ina);
	uint16_t config = 0;
	// Example: 32V range, Gain /8 (320mV), 12-bit ADC, continuous shunt+bus
	config = INA_CFG_BRNG_BIT | INA_CFG_PG_BITS(3) |
					  INA_CFG_BADC_BITS(0x03) | INA_CFG_SADC_BITS(0x03) |
					  INA_CFG_MODE_BITS(INA_MODE_SHUNT_BUS_CONT);

	eStatus |= INA219_WriteRegister(ina, INA_CFG_REG, config);

	return eStatus;
}

HAL_StatusTypeDef INA219_SetCalibration(INA219_HandleTypeDef *ina, float r_shunt, float i_max)
{	
	HAL_StatusTypeDef eStatus;

    // 1️ Tính khoảng LSB hợp lệ
    float min_lsb = i_max / 32767.0f;   // Minimum LSB
    float max_lsb = i_max / 4096.0f;    // Maximum LSB

    // 2️Chọn Current LSB — làm tròn lên giá trị đẹp gần min_lsb (ví dụ bội của 1e-6)
    float step = 1e-6f;
    float current_lsb = ceilf(min_lsb / step) * step;
    if (current_lsb > max_lsb)
        current_lsb = max_lsb / 2.0f;   // fallback nếu vượt giới hạn

    // 3️Ghi vào struct
    ina->shunt_resistance = r_shunt;
    ina->current_lsb = current_lsb;
    ina->power_lsb = current_lsb * 20.0f;

    // 4️ Tính Calibration register
    // CAL = 0.04096 / (current_LSB * Rshunt)
    float cal_f = 0.04096f / (current_lsb * r_shunt);
    if (cal_f > 65535.0f) cal_f = 65535.0f;
    if (cal_f < 1.0f) cal_f = 1.0f;
    ina->calib_value = (uint16_t)cal_f;

    // 5️ Ghi vào thanh ghi CALIB
    eStatus = INA219_WriteRegister(ina, INA_CALIB_REG, ina->calib_value);

    return eStatus;
}


float INA219_ReadBusVoltage(INA219_HandleTypeDef *ina)
{
	uint16_t value = -12;
	HAL_StatusTypeDef eStatus;
	eStatus = INA219_ReadRegister(ina, INA_BUS_VOL_REG, &value);
	if(eStatus == HAL_OK)
	{
		value >>= 3; // Bit [2:0] are flags
		return value * 0.004f; // 4mV per bit
	}
	return value;
}

float INA219_ReadShuntVoltage(INA219_HandleTypeDef *ina)
{
    uint16_t raw;
    int16_t value;
    HAL_StatusTypeDef eStatus;
    // Đọc 2 byte từ thanh ghi Shunt Voltage (0x01)
    eStatus = INA219_ReadRegister(ina, INA_SHT_VOL_REG, &raw);

    if(eStatus == HAL_OK)
    {
	    // Chuyển về signed 16-bit (Two’s complement)
	    value = (int16_t)raw;

	    // Mỗi bit = 10 µV = 0.00001 V (theo datasheet)
	    float voltage = (float)value * 0.00001f;

	    return voltage;  // đơn vị: Volt    	
    }
    return -12;
}

float INA219_ReadCurrent(INA219_HandleTypeDef *ina)
{
    uint16_t raw;
    int16_t value;
    HAL_StatusTypeDef eStatus;

    // Đọc thanh ghi Current (0x04)
    eStatus = INA219_ReadRegister(ina, INA_CUR_REG, &raw);
    if (eStatus == HAL_OK)
    {
        // Chuyển về signed 16-bit
        value = (int16_t)raw;

        // Nhân với current_lsb để ra dòng thực tế (A)
        float current = (float)value * ina->current_lsb;
        return current;
    }
    return -12.0f; // Trả lỗi
}

float INA219_ReadPower(INA219_HandleTypeDef *ina)
{
    uint16_t raw;
    HAL_StatusTypeDef eStatus;

    // Đọc thanh ghi Power (0x03)
    eStatus = INA219_ReadRegister(ina, INA_PWR_REG, &raw);
    if (eStatus == HAL_OK)
    {
        // Mỗi đơn vị = 20 × Current_LSB
        float power = (float)raw * ina->power_lsb;
        return power; // đơn vị: Watt
    }
    return -12.0f; // Trả lỗi
}
