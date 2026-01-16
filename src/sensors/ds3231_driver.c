/**
 * @file ds3231_driver.c
 * @brief Драйвер для DS3231 - модуль часов реального времени (STM32F103)
 */

#include "sensors/ds3231.h"
#include "stm32f1xx_hal.h"
#include <string.h>

// Внешний I2C handle (должен быть инициализирован в main.c)
extern I2C_HandleTypeDef hi2c1;

// Внутренние регистры DS3231
#define DS3231_REG_SECONDS     0x00
#define DS3231_REG_MINUTES     0x01
#define DS3231_REG_HOURS       0x02
#define DS3231_REG_DAY         0x03
#define DS3231_REG_DATE        0x04
#define DS3231_REG_MONTH       0x05
#define DS3231_REG_YEAR        0x06
#define DS3231_REG_TEMP_MSB    0x11
#define DS3231_REG_TEMP_LSB    0x12
#define DS3231_REG_STATUS      0x0F

// Вспомогательные функции для конвертации BCD <-> Decimal
static uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) + (dec % 10);
}

// Чтение регистра DS3231
static bool ds3231_read_register(uint8_t reg, uint8_t* data) {
    if (HAL_I2C_Mem_Read(&hi2c1, DS3231_I2C_ADDRESS << 1, reg, 1, data, 1, HAL_MAX_DELAY) == HAL_OK) {
        return true;
    }
    return false;
}

// Запись регистра DS3231
static bool ds3231_write_register(uint8_t reg, uint8_t data) {
    if (HAL_I2C_Mem_Write(&hi2c1, DS3231_I2C_ADDRESS << 1, reg, 1, &data, 1, HAL_MAX_DELAY) == HAL_OK) {
        return true;
    }
    return false;
}

bool ds3231_init(void) {
    // Проверка подключения (чтение регистра статуса)
    uint8_t status;
    if (!ds3231_read_register(DS3231_REG_STATUS, &status)) {
        return false;  // DS3231 не отвечает
    }
    
    return true;
}

bool ds3231_set_datetime(ds3231_datetime_t* datetime) {
    if (datetime == NULL) {
        return false;
    }
    
    // Проверка валидности данных
    if (datetime->seconds > 59 || datetime->minutes > 59 || 
        datetime->hours > 23 || datetime->date > 31 || 
        datetime->month > 12 || datetime->day > 7) {
        return false;
    }
    
    // Запись времени и даты (в формате BCD)
    if (!ds3231_write_register(DS3231_REG_SECONDS, dec_to_bcd(datetime->seconds))) return false;
    if (!ds3231_write_register(DS3231_REG_MINUTES, dec_to_bcd(datetime->minutes))) return false;
    if (!ds3231_write_register(DS3231_REG_HOURS, dec_to_bcd(datetime->hours))) return false;
    if (!ds3231_write_register(DS3231_REG_DAY, dec_to_bcd(datetime->day))) return false;
    if (!ds3231_write_register(DS3231_REG_DATE, dec_to_bcd(datetime->date))) return false;
    if (!ds3231_write_register(DS3231_REG_MONTH, dec_to_bcd(datetime->month))) return false;
    if (!ds3231_write_register(DS3231_REG_YEAR, dec_to_bcd(datetime->year))) return false;
    
    return true;
}

bool ds3231_get_datetime(ds3231_datetime_t* datetime) {
    if (datetime == NULL) {
        return false;
    }
    
    uint8_t data[7];
    
    // Чтение всех регистров времени и даты
    if (HAL_I2C_Mem_Read(&hi2c1, DS3231_I2C_ADDRESS << 1, DS3231_REG_SECONDS, 1, data, 7, HAL_MAX_DELAY) != HAL_OK) {
        return false;
    }
    
    // Конвертация из BCD в decimal
    datetime->seconds = bcd_to_dec(data[0] & 0x7F);
    datetime->minutes = bcd_to_dec(data[1] & 0x7F);
    datetime->hours = bcd_to_dec(data[2] & 0x3F);
    datetime->day = bcd_to_dec(data[3] & 0x07);
    datetime->date = bcd_to_dec(data[4] & 0x3F);
    datetime->month = bcd_to_dec(data[5] & 0x1F);
    datetime->year = bcd_to_dec(data[6]);
    
    return true;
}

bool ds3231_get_temperature(ds3231_temperature_t* temp) {
    if (temp == NULL) {
        return false;
    }
    
    uint8_t msb, lsb;
    
    // Чтение регистров температуры
    if (!ds3231_read_register(DS3231_REG_TEMP_MSB, &msb)) return false;
    if (!ds3231_read_register(DS3231_REG_TEMP_LSB, &lsb)) return false;
    
    // Температура в формате: MSB = целая часть, LSB = дробная (0.25°C шаг)
    temp->integer = (int8_t)msb;
    temp->fraction = (lsb >> 6) * 25;  // 00=0, 01=25, 10=50, 11=75
    
    return true;
}

float ds3231_get_temperature_float(void) {
    ds3231_temperature_t temp;
    if (ds3231_get_temperature(&temp)) {
        return (float)temp.integer + ((float)temp.fraction / 100.0f);
    }
    return -999.0f;
}

bool ds3231_is_connected(void) {
    uint8_t status;
    return ds3231_read_register(DS3231_REG_STATUS, &status);
}


