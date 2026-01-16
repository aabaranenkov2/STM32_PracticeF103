#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>
#include <stdbool.h>

// I2C адрес DS3231 (фиксированный)
#define DS3231_I2C_ADDRESS 0x68

// Структура для хранения даты и времени
typedef struct {
    uint8_t seconds;     // 0-59
    uint8_t minutes;     // 0-59
    uint8_t hours;       // 0-23 (24-часовой формат)
    uint8_t day;         // 1-7 (1=воскресенье, 7=суббота)
    uint8_t date;        // 1-31
    uint8_t month;       // 1-12
    uint8_t year;        // 0-99 (0=2000, 99=2099)
} ds3231_datetime_t;

// Структура для хранения температуры
typedef struct {
    int8_t integer;      // Целая часть температуры (°C)
    uint8_t fraction;    // Дробная часть (0.25°C шаг)
} ds3231_temperature_t;

/**
 * @brief Инициализация DS3231
 */
bool ds3231_init(void);

/**
 * @brief Установка текущей даты и времени
 */
bool ds3231_set_datetime(ds3231_datetime_t* datetime);

/**
 * @brief Чтение текущей даты и времени
 */
bool ds3231_get_datetime(ds3231_datetime_t* datetime);

/**
 * @brief Чтение температуры с встроенного термометра DS3231
 */
bool ds3231_get_temperature(ds3231_temperature_t* temp);

/**
 * @brief Получить температуру в виде float
 */
float ds3231_get_temperature_float(void);

/**
 * @brief Проверка, работает ли DS3231
 */
bool ds3231_is_connected(void);

#endif // DS3231_H


