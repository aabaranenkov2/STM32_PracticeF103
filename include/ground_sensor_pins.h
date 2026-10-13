#ifndef GROUND_SENSOR_PINS_H
#define GROUND_SENSOR_PINS_H

#include "stm32f1xx_hal.h"

#include <stdbool.h>

/* LG8A3 на PB12. В прошивке сигнал инвертирован:
 * сырой LOW на PB12 (0 В, металл) → active=true, LED ON. */
#define GROUND_SENSOR_PORT         GPIOB
#define GROUND_SENSOR_PIN          GPIO_PIN_12

#define STATUS_LED_PORT            GPIOC
#define STATUS_LED_PIN             GPIO_PIN_13

static inline GPIO_PinState ground_sensor_pin_read(void)
{
    return HAL_GPIO_ReadPin(GROUND_SENSOR_PORT, GROUND_SENSOR_PIN);
}

static inline bool ground_sensor_raw_active(void)
{
    return ground_sensor_pin_read() == GPIO_PIN_RESET;
}

/* PC13 ← инверсия PB12 напрямую, без задержек и без автомата. */
static inline void ground_sensor_led_mirror(void)
{
    const GPIO_PinState pb12 = ground_sensor_pin_read();

    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN,
                      (pb12 == GPIO_PIN_RESET) ? GPIO_PIN_RESET :
                      GPIO_PIN_SET);
}

static inline void ground_sensor_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio.Pin = GROUND_SENSOR_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GROUND_SENSOR_PORT, &gpio);

    gpio.Pin = STATUS_LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STATUS_LED_PORT, &gpio);

    ground_sensor_led_mirror();
}

#endif
