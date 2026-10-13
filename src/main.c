#include "stm32f1xx_hal.h"

#define STATUS_LED_PORT GPIOC
#define STATUS_LED_PIN  GPIO_PIN_13
#define BLINK_PERIOD_MS 500U

static void gpio_init(void);

void SysTick_Handler(void)
{
    HAL_IncTick();
}

int main(void)
{
    uint32_t last_toggle_ms;

    HAL_Init();
    gpio_init();
    last_toggle_ms = HAL_GetTick();

    for (;;)
    {
        const uint32_t now_ms = HAL_GetTick();

        if ((now_ms - last_toggle_ms) >= BLINK_PERIOD_MS)
        {
            last_toggle_ms = now_ms;
            HAL_GPIO_TogglePin(STATUS_LED_PORT, STATUS_LED_PIN);
        }
    }
}

static void gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_PIN_SET);

    gpio.Pin = STATUS_LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STATUS_LED_PORT, &gpio);
}
