/* Тест LG8A3: PB12 → инверсия → PC13, без задержек. */
#include "ground_sensor_pins.h"

#include <stdint.h>

static void system_clock_config(void);
static void boot_blink(void);

void SysTick_Handler(void)
{
    HAL_IncTick();
}

int main(void)
{
    HAL_Init();
    system_clock_config();
    ground_sensor_gpio_init();
    boot_blink();

    for (;;)
    {
        ground_sensor_led_mirror();
    }
}

static void boot_blink(void)
{
    uint8_t i;

    for (i = 0U; i < 5U; ++i)
    {
        HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_PIN_RESET);
        HAL_Delay(80);
        HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_PIN_SET);
        HAL_Delay(80);
    }
    HAL_Delay(300);
}

static void system_clock_config(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clocks = {0};

    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSE |
                                RCC_OSCILLATORTYPE_HSI;
    oscillator.HSEState = RCC_HSE_ON;
    oscillator.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    oscillator.HSIState = RCC_HSI_ON;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscillator.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK)
    {
        oscillator.PLL.PLLState = RCC_PLL_OFF;
        oscillator.HSEState = RCC_HSE_OFF;
        (void)HAL_RCC_OscConfig(&oscillator);

        clocks.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                           RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
        clocks.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
        clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
        clocks.APB1CLKDivider = RCC_HCLK_DIV1;
        clocks.APB2CLKDivider = RCC_HCLK_DIV1;
        (void)HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_0);
        return;
    }

    clocks.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clocks.APB1CLKDivider = RCC_HCLK_DIV2;
    clocks.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_2) != HAL_OK)
    {
        for (;;)
        {
            __NOP();
        }
    }
}
