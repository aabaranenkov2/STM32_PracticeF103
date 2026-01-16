/**
 * Проект для STM32F103C8T6 (Blue Pill)
 * Nextion дисплей + DS3231 RTC
 * Отображение времени и температуры, установка времени
 */

#include "stm32f1xx_hal.h"
#include "ui/nextion_display.h"
#include "sensors/ds3231.h"
#include <stdio.h>
#include <string.h>

// Прототипы функций
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
void UpdateDisplay(void);
void HandleNextionTouch(uint8_t page_id, uint8_t component_id, uint8_t event);
void AdjustTimeField(uint8_t field, int8_t delta);
void SaveTime(void);

// Handles для периферии
UART_HandleTypeDef huart2;
I2C_HandleTypeDef hi2c1;

// Буфер для приема данных от Nextion
uint8_t nextion_rx_buffer[32];
uint16_t nextion_rx_index = 0;
volatile uint8_t nextion_data_ready = 0;

// Флаги для обновления дисплея
volatile uint32_t last_display_update = 0;
#define DISPLAY_UPDATE_INTERVAL 500  // Обновление каждые 500 мс

// Флаг режима установки времени
volatile uint8_t time_set_mode = 0;
volatile uint8_t time_set_field = 0;  // 0=час, 1=минута, 2=секунда

int main(void) {
    // Инициализация HAL
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();
    
    // Инициализация DS3231
    if (!ds3231_init()) {
        // Ошибка инициализации DS3231
        // Можно мигнуть LED для индикации ошибки
        while(1) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            HAL_Delay(200);
        }
    }
    
    // Инициализация дисплея Nextion
    if (Nextion_Init(&huart2) != HAL_OK) {
        // Ошибка инициализации Nextion
        while(1) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            HAL_Delay(100);
        }
    }
    
    // Установка яркости дисплея
    Nextion_SetBrightness(80);
    
    // Переключение на страницу 0 (главная)
    Nextion_SetPage(0);
    HAL_Delay(100);
    
    // Начало приема данных от Nextion
    HAL_UART_Receive_IT(&huart2, nextion_rx_buffer, 32);
    
    // Главный цикл
    while (1) {
        // Обновление дисплея каждые 500 мс
        if (HAL_GetTick() - last_display_update >= DISPLAY_UPDATE_INTERVAL) {
            last_display_update = HAL_GetTick();
            
            if (!time_set_mode) {
                // Режим отображения времени и температуры
                UpdateDisplay();
            }
        }
        
        // Обработка данных от Nextion
        if (nextion_data_ready) {
            // Проверка на завершающую последовательность
            if (nextion_rx_index >= 3 && 
                nextion_rx_buffer[nextion_rx_index - 3] == 0xFF && 
                nextion_rx_buffer[nextion_rx_index - 2] == 0xFF && 
                nextion_rx_buffer[nextion_rx_index - 1] == 0xFF) {
                
                // Событие касания (Touch Event)
                if (nextion_rx_buffer[0] == 0x65 && nextion_rx_index >= 6) {
                    uint8_t page_id = nextion_rx_buffer[1];
                    uint8_t component_id = nextion_rx_buffer[2];
                    uint8_t event = nextion_rx_buffer[3];
                    
                    HandleNextionTouch(page_id, component_id, event);
                }
            }
            
            nextion_data_ready = 0;
            nextion_rx_index = 0;
            HAL_UART_Receive_IT(&huart2, nextion_rx_buffer, 32);
        }
        
        HAL_Delay(10);
    }
}

/**
 * @brief Обновление дисплея (время и температура)
 */
void UpdateDisplay(void) {
    ds3231_datetime_t datetime;
    ds3231_temperature_t temp;
    char buffer[64];
    
    // Чтение времени
    if (ds3231_get_datetime(&datetime)) {
        // Форматирование времени: HH:MM:SS
        sprintf(buffer, "%02d:%02d:%02d", datetime.hours, datetime.minutes, datetime.seconds);
        Nextion_SetText("t_time", buffer);
        
        // Форматирование даты: DD.MM.YY
        sprintf(buffer, "%02d.%02d.20%02d", datetime.date, datetime.month, datetime.year);
        Nextion_SetText("t_date", buffer);
    }
    
    // Чтение температуры
    if (ds3231_get_temperature(&temp)) {
        sprintf(buffer, "%d.%02d°C", temp.integer, temp.fraction);
        Nextion_SetText("t_temp", buffer);
    }
}

/**
 * @brief Обработка событий касаний от Nextion
 */
void HandleNextionTouch(uint8_t page_id, uint8_t component_id, uint8_t event) {
    // event: 0x00 = press, 0x01 = release
    
    if (event == 0x01) {  // Release (отпускание)
        if (page_id == 0) {  // Главная страница
            if (component_id == 0) {  // Кнопка "Settings"
                // Переход на страницу установки времени
                time_set_mode = 1;
                Nextion_SetPage(1);  // Страница установки времени
                time_set_field = 0;  // Начинаем с часов
            }
        }
        else if (page_id == 1) {  // Страница установки времени
            if (component_id == 0) {  // Кнопка "Hour +"
                AdjustTimeField(0, 1);
            }
            else if (component_id == 1) {  // Кнопка "Hour -"
                AdjustTimeField(0, -1);
            }
            else if (component_id == 2) {  // Кнопка "Minute +"
                AdjustTimeField(1, 1);
            }
            else if (component_id == 3) {  // Кнопка "Minute -"
                AdjustTimeField(1, -1);
            }
            else if (component_id == 4) {  // Кнопка "Second +"
                AdjustTimeField(2, 1);
            }
            else if (component_id == 5) {  // Кнопка "Second -"
                AdjustTimeField(2, -1);
            }
            else if (component_id == 6) {  // Кнопка "Save"
                SaveTime();
                time_set_mode = 0;
                Nextion_SetPage(0);  // Возврат на главную
            }
            else if (component_id == 7) {  // Кнопка "Cancel"
                time_set_mode = 0;
                Nextion_SetPage(0);  // Возврат на главную
            }
        }
    }
}

/**
 * @brief Корректировка поля времени
 */
void AdjustTimeField(uint8_t field, int8_t delta) {
    // Читаем текущее время
    if (!ds3231_get_datetime(&time_set_datetime)) {
        return;
    }
    
    switch(field) {
        case 0:  // Часы
            if (delta > 0) {
                time_set_datetime.hours++;
                if (time_set_datetime.hours > 23) time_set_datetime.hours = 0;
            } else {
                if (time_set_datetime.hours == 0) time_set_datetime.hours = 23;
                else time_set_datetime.hours--;
            }
            break;
        case 1:  // Минуты
            if (delta > 0) {
                time_set_datetime.minutes++;
                if (time_set_datetime.minutes > 59) time_set_datetime.minutes = 0;
            } else {
                if (time_set_datetime.minutes == 0) time_set_datetime.minutes = 59;
                else time_set_datetime.minutes--;
            }
            break;
        case 2:  // Секунды
            if (delta > 0) {
                time_set_datetime.seconds++;
                if (time_set_datetime.seconds > 59) time_set_datetime.seconds = 0;
            } else {
                if (time_set_datetime.seconds == 0) time_set_datetime.seconds = 59;
                else time_set_datetime.seconds--;
            }
            break;
    }
    
    // Обновление на дисплее
    char buffer[32];
    sprintf(buffer, "%02d", time_set_datetime.hours);
    Nextion_SetText("t_hour", buffer);
    sprintf(buffer, "%02d", time_set_datetime.minutes);
    Nextion_SetText("t_minute", buffer);
    sprintf(buffer, "%02d", time_set_datetime.seconds);
    Nextion_SetText("t_second", buffer);
}

// Глобальная переменная для хранения времени при установке
static ds3231_datetime_t time_set_datetime;

/**
 * @brief Сохранение времени в DS3231
 */
void SaveTime(void) {
    // Используем сохраненное время из AdjustTimeField
    ds3231_set_datetime(&time_set_datetime);
}

// Переопределение Nextion_ProcessData для обработки касаний
// (Эта функция уже определена в nextion_display.c, но мы можем добавить обработку здесь)
// Для этого нужно изменить nextion_display.c или создать callback

// Callback для приема данных от UART
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        // Ищем завершающую последовательность 0xFF 0xFF 0xFF
        for (uint16_t i = 0; i < 32 - 2; i++) {
            if (nextion_rx_buffer[i] == 0xFF && 
                nextion_rx_buffer[i+1] == 0xFF && 
                nextion_rx_buffer[i+2] == 0xFF) {
                nextion_rx_index = i + 3;
                nextion_data_ready = 1;
                return;
            }
        }
        // Если не найдено, продолжаем прием
        HAL_UART_Receive_IT(&huart2, nextion_rx_buffer, 32);
    }
}

// Обработчик SysTick
void SysTick_Handler(void) {
    HAL_IncTick();
}

// Настройка системных часов (72 MHz для STM32F103)
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
    
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

// Настройка GPIO
static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // PC13 - LED
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

// Настройка UART2 для Nextion
static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // PA2 - USART2_TX, PA3 - USART2_RX
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    HAL_UART_Init(&huart2);
}

// Настройка I2C1 для DS3231
static void MX_I2C1_Init(void) {
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // PB6 - I2C1_SCL, PB7 - I2C1_SDA
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    HAL_I2C_Init(&hi2c1);
}
