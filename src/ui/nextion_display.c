/**
 * @file nextion_display.c
 * @brief Реализация драйвера для дисплея Nextion (STM32F103)
 */

#include "ui/nextion_display.h"
#include <string.h>
#include <stdio.h>

// Внешний UART handle (должен быть определен в main.c)
extern UART_HandleTypeDef huart2;

// Внутренняя переменная для хранения UART handle
static UART_HandleTypeDef *nextion_uart = NULL;

/**
 * @brief Инициализация дисплея Nextion
 */
HAL_StatusTypeDef Nextion_Init(UART_HandleTypeDef *huart) {
    if (huart == NULL) {
        return HAL_ERROR;
    }
    
    nextion_uart = huart;
    
    // Задержка для инициализации дисплея
    HAL_Delay(100);
    
    // Сброс дисплея
    HAL_Delay(100);
    Nextion_Reset();
    HAL_Delay(500);
    
    // Установка яркости (100%)
    Nextion_SetBrightness(100);
    
    return HAL_OK;
}

/**
 * @brief Отправка сырых данных на дисплей
 */
static HAL_StatusTypeDef Nextion_SendRaw(const uint8_t *data, uint16_t length) {
    if (nextion_uart == NULL || data == NULL) {
        return HAL_ERROR;
    }
    
    return HAL_UART_Transmit(nextion_uart, (uint8_t *)data, length, NEXTION_TIMEOUT);
}

/**
 * @brief Отправка завершающей последовательности (0xFF 0xFF 0xFF)
 */
static void Nextion_SendEnd(void) {
    const uint8_t end_cmd[] = {NEXTION_CMD_END};
    HAL_UART_Transmit(nextion_uart, (uint8_t *)end_cmd, 3, NEXTION_TIMEOUT);
}

/**
 * @brief Отправка команды на дисплей
 */
HAL_StatusTypeDef Nextion_SendCommand(const char *command) {
    if (nextion_uart == NULL || command == NULL) {
        return HAL_ERROR;
    }
    
    uint16_t len = strlen(command);
    HAL_StatusTypeDef status = Nextion_SendRaw((uint8_t *)command, len);
    
    if (status == HAL_OK) {
        Nextion_SendEnd();
    }
    
    return status;
}

/**
 * @brief Установка значения компонента
 */
HAL_StatusTypeDef Nextion_SetValue(const char *component, const char *attribute, uint32_t value) {
    if (component == NULL || attribute == NULL) {
        return HAL_ERROR;
    }
    
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "%s%s%lu", component, attribute, (unsigned long)value);
    
    return Nextion_SendCommand(cmd);
}

/**
 * @brief Установка текста компонента
 */
HAL_StatusTypeDef Nextion_SetText(const char *component, const char *text) {
    if (component == NULL || text == NULL) {
        return HAL_ERROR;
    }
    
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "%s%s%s\"", component, NEXTION_CMD_SET_TEXT, text);
    
    return Nextion_SendCommand(cmd);
}

/**
 * @brief Установка яркости дисплея
 */
HAL_StatusTypeDef Nextion_SetBrightness(uint8_t brightness) {
    if (brightness > 100) {
        brightness = 100;
    }
    
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "%s%u", NEXTION_CMD_BRIGHTNESS, brightness);
    
    return Nextion_SendCommand(cmd);
}

/**
 * @brief Переключение страницы
 */
HAL_StatusTypeDef Nextion_SetPage(uint8_t page) {
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "%s%u", NEXTION_CMD_PAGE, page);
    
    return Nextion_SendCommand(cmd);
}

/**
 * @brief Сброс дисплея
 */
HAL_StatusTypeDef Nextion_Reset(void) {
    return Nextion_SendCommand(NEXTION_CMD_RESET);
}

/**
 * @brief Режим сна
 */
HAL_StatusTypeDef Nextion_Sleep(uint8_t enable) {
    char cmd[16];
    snprintf(cmd, sizeof(cmd), "%s%u", NEXTION_CMD_SLEEP, enable ? 1 : 0);
    
    return Nextion_SendCommand(cmd);
}

/**
 * @brief Обработка данных от дисплея
 * 
 * Формат: 0x65 [page_id] [component_id] [event] 0xFF 0xFF 0xFF
 * event: 0x01 = release, 0x00 = press
 */
void Nextion_ProcessData(uint8_t *data, uint16_t length) {
    if (data == NULL || length == 0) {
        return;
    }
    
    // Проверка на завершающую последовательность
    if (length >= 3 && 
        data[length - 3] == 0xFF && 
        data[length - 2] == 0xFF && 
        data[length - 1] == 0xFF) {
        
        // Событие касания (Touch Event)
        if (data[0] == 0x65 && length >= 6) {
            uint8_t page_id = data[1];
            uint8_t component_id = data[2];
            uint8_t event = data[3];  // 0x01 = release, 0x00 = press
            
            // Здесь можно обработать событие касания
            // Например, вызвать callback функцию
        }
    }
}


