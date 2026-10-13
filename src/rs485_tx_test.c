#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

#define RS485_DE_PORT       GPIOB
#define RS485_DE_PIN        GPIO_PIN_0
#define STATUS_LED_PORT     GPIOC
#define STATUS_LED_PIN      GPIO_PIN_13
#define TX_PERIOD_MS        500U

static UART_HandleTypeDef g_uart1;
static volatile bool g_tx_busy;
static const uint8_t g_test_frame[10] = {
    0x55U, 0xAAU, 0x81U, 0x04U, 0xDEU,
    0xADU, 0xBEU, 0xEFU, 0xFBU, 0x36U
};

static void system_clock_config(void);
static void gpio_init(void);
static void uart1_init(void);

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_uart1);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1)
    {
        HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
        HAL_GPIO_TogglePin(STATUS_LED_PORT, STATUS_LED_PIN);
        g_tx_busy = false;
    }
}

int main(void)
{
    uint32_t next_transmission;

    HAL_Init();
    system_clock_config();
    gpio_init();
    uart1_init();
    next_transmission = HAL_GetTick() + TX_PERIOD_MS;

    for (;;)
    {
        if (!g_tx_busy &&
            ((int32_t)(HAL_GetTick() - next_transmission) >= 0))
        {
            g_tx_busy = true;
            next_transmission += TX_PERIOD_MS;
            HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_SET);
            if (HAL_UART_Transmit_IT(&g_uart1, (uint8_t *)g_test_frame,
                                     sizeof(g_test_frame)) != HAL_OK)
            {
                HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN,
                                  GPIO_PIN_RESET);
                g_tx_busy = false;
            }
        }
        __WFI();
    }
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
        for (;;)
        {
            __NOP();
        }
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

static void gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_PIN_SET);

    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = RS485_DE_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RS485_DE_PORT, &gpio);

    gpio.Pin = STATUS_LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STATUS_LED_PORT, &gpio);
}

static void uart1_init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    g_uart1.Instance = USART1;
    g_uart1.Init.BaudRate = 9600U;
    g_uart1.Init.WordLength = UART_WORDLENGTH_8B;
    g_uart1.Init.StopBits = UART_STOPBITS_1;
    g_uart1.Init.Parity = UART_PARITY_NONE;
    g_uart1.Init.Mode = UART_MODE_TX;
    g_uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_uart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&g_uart1) != HAL_OK)
    {
        for (;;)
        {
            __NOP();
        }
    }

    HAL_NVIC_SetPriority(USART1_IRQn, 1U, 0U);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}
