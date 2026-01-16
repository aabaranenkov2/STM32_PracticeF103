/**
 * @file nextion_display.h
 * @brief Драйвер для дисплея Nextion NX4832T035_011 (STM32F103)
 * 
 * Nextion - интеллектуальный TFT дисплей с управлением через UART
 * 
 * ⚠️ ВАЖНО: Мультитач НЕ поддерживается (резистивный сенсор)
 */

#ifndef NEXTION_DISPLAY_H
#define NEXTION_DISPLAY_H

#include "stm32f1xx_hal.h"

// Конфигурация UART для Nextion
#define NEXTION_UART_HANDLE     huart2  // UART2 для STM32F103
#define NEXTION_BAUDRATE        9600     // Скорость по умолчанию
#define NEXTION_TIMEOUT         1000     // Таймаут в мс

// Команды Nextion (завершаются 0xFF 0xFF 0xFF)
#define NEXTION_CMD_END         0xFF, 0xFF, 0xFF

// Базовые команды
#define NEXTION_CMD_RESET       "rest"
#define NEXTION_CMD_SLEEP       "sleep="
#define NEXTION_CMD_BRIGHTNESS   "dim="
#define NEXTION_CMD_PAGE         "page "

// Команды для компонентов
#define NEXTION_CMD_SET_TEXT     ".txt=\""
#define NEXTION_CMD_SET_VAL      ".val="
#define NEXTION_CMD_SET_PIC      ".pic="
#define NEXTION_CMD_SET_PCO      ".pco="
#define NEXTION_CMD_SET_BCO      ".bco="

/**
 * @brief Инициализация дисплея Nextion
 */
HAL_StatusTypeDef Nextion_Init(UART_HandleTypeDef *huart);

/**
 * @brief Отправка команды на дисплей
 */
HAL_StatusTypeDef Nextion_SendCommand(const char *command);

/**
 * @brief Установка значения компонента
 */
HAL_StatusTypeDef Nextion_SetValue(const char *component, const char *attribute, uint32_t value);

/**
 * @brief Установка текста компонента
 */
HAL_StatusTypeDef Nextion_SetText(const char *component, const char *text);

/**
 * @brief Установка яркости дисплея
 */
HAL_StatusTypeDef Nextion_SetBrightness(uint8_t brightness);

/**
 * @brief Переключение страницы
 */
HAL_StatusTypeDef Nextion_SetPage(uint8_t page);

/**
 * @brief Сброс дисплея
 */
HAL_StatusTypeDef Nextion_Reset(void);

/**
 * @brief Режим сна
 */
HAL_StatusTypeDef Nextion_Sleep(uint8_t enable);

/**
 * @brief Обработка данных от дисплея (события касаний)
 */
void Nextion_ProcessData(uint8_t *data, uint16_t length);

#endif // NEXTION_DISPLAY_H


