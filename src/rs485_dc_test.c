#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

#define RS485_DI_PORT       GPIOA
#define RS485_DI_PIN        GPIO_PIN_9
#define RS485_DE_PORT       GPIOB
#define RS485_DE_PIN        GPIO_PIN_0
#define STATUS_LED_PORT     GPIOC
#define STATUS_LED_PIN      GPIO_PIN_13
#define TOGGLE_PERIOD_MS    3000U

void SysTick_Handler(void)
{
    HAL_IncTick();
}

int main(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint32_t next_toggle;
    bool output_high = false;

    HAL_Init();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio.Pin = RS485_DI_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RS485_DI_PORT, &gpio);

    gpio.Pin = RS485_DE_PIN;
    HAL_GPIO_Init(RS485_DE_PORT, &gpio);

    gpio.Pin = STATUS_LED_PIN;
    HAL_GPIO_Init(STATUS_LED_PORT, &gpio);

    HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RS485_DI_PORT, RS485_DI_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_PIN_RESET);
    next_toggle = HAL_GetTick() + TOGGLE_PERIOD_MS;

    for (;;)
    {
        if ((int32_t)(HAL_GetTick() - next_toggle) >= 0)
        {
            next_toggle += TOGGLE_PERIOD_MS;
            output_high = !output_high;
            HAL_GPIO_WritePin(RS485_DI_PORT, RS485_DI_PIN,
                              output_high ? GPIO_PIN_SET : GPIO_PIN_RESET);
            HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN,
                              output_high ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
        __WFI();
    }
}
