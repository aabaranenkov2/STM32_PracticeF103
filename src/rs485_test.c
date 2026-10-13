#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

#define RS485_DE_PORT       GPIOB
#define RS485_DE_PIN        GPIO_PIN_0
#define STATUS_LED_PORT     GPIOC
#define STATUS_LED_PIN      GPIO_PIN_13
#define FRAME_SIZE          10U
#define RS485_BAUD_RATE     9600U
#define DMA_RX_SIZE         32U
#define RX_RING_SIZE        64U

typedef struct
{
    uint32_t received_bytes;
    uint32_t valid_requests;
    uint32_t transmitted_frames;
    uint32_t uart_errors;
} rs485_diagnostics_t;

volatile rs485_diagnostics_t g_rs485_diagnostics;

static UART_HandleTypeDef g_uart1;
static DMA_HandleTypeDef g_dma_uart1_rx;
static volatile bool g_tx_busy;
static volatile bool g_response_pending;
static volatile uint8_t g_rx_write;
static volatile uint8_t g_rx_read;
static uint32_t g_response_due_ms;
static uint8_t g_dma_rx[DMA_RX_SIZE];
static uint8_t g_rx_ring[RX_RING_SIZE];
static uint8_t g_request[FRAME_SIZE];
static uint8_t g_response[FRAME_SIZE];
static uint8_t g_request_length;

static void system_clock_config(void);
static void gpio_init(void);
static void uart1_init(void);
static uint16_t crc16_modbus(const uint8_t *data, uint8_t length);
static void process_received_byte(uint8_t value);
static void process_request(void);

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_uart1);
}

void DMA1_Channel5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_dma_uart1_rx);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *uart, uint16_t size)
{
    if (uart->Instance == USART1)
    {
        uint16_t index;

        for (index = 0U; index < size; ++index)
        {
            const uint8_t next = (uint8_t)((g_rx_write + 1U) % RX_RING_SIZE);

            if (next != g_rx_read)
            {
                g_rx_ring[g_rx_write] = g_dma_rx[index];
                g_rx_write = next;
                ++g_rs485_diagnostics.received_bytes;
            }
        }
        (void)HAL_UARTEx_ReceiveToIdle_DMA(&g_uart1, g_dma_rx, DMA_RX_SIZE);
        __HAL_DMA_DISABLE_IT(&g_dma_uart1_rx, DMA_IT_HT);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1)
    {
        HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
        g_tx_busy = false;
        ++g_rs485_diagnostics.transmitted_frames;
        HAL_GPIO_TogglePin(STATUS_LED_PORT, STATUS_LED_PIN);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1)
    {
        ++g_rs485_diagnostics.uart_errors;
        __HAL_UART_CLEAR_OREFLAG(&g_uart1);
        (void)HAL_UART_AbortReceive(&g_uart1);
        (void)HAL_UARTEx_ReceiveToIdle_DMA(&g_uart1, g_dma_rx, DMA_RX_SIZE);
        __HAL_DMA_DISABLE_IT(&g_dma_uart1_rx, DMA_IT_HT);
    }
}

int main(void)
{
    HAL_Init();
    system_clock_config();
    gpio_init();
    uart1_init();
    (void)HAL_UARTEx_ReceiveToIdle_DMA(&g_uart1, g_dma_rx, DMA_RX_SIZE);
    __HAL_DMA_DISABLE_IT(&g_dma_uart1_rx, DMA_IT_HT);

    for (;;)
    {
        while (g_rx_read != g_rx_write)
        {
            const uint8_t value = g_rx_ring[g_rx_read];
            g_rx_read = (uint8_t)((g_rx_read + 1U) % RX_RING_SIZE);
            process_received_byte(value);
        }
        if (g_response_pending &&
            ((int32_t)(HAL_GetTick() - g_response_due_ms) >= 0))
        {
            g_response_pending = false;
            HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_SET);
            if (HAL_UART_Transmit_IT(&g_uart1, g_response,
                                     FRAME_SIZE) != HAL_OK)
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

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
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
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    g_dma_uart1_rx.Instance = DMA1_Channel5;
    g_dma_uart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    g_dma_uart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    g_dma_uart1_rx.Init.MemInc = DMA_MINC_ENABLE;
    g_dma_uart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    g_dma_uart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    g_dma_uart1_rx.Init.Mode = DMA_NORMAL;
    g_dma_uart1_rx.Init.Priority = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&g_dma_uart1_rx) != HAL_OK)
    {
        for (;;)
        {
            __NOP();
        }
    }

    g_uart1.Instance = USART1;
    g_uart1.Init.BaudRate = RS485_BAUD_RATE;
    g_uart1.Init.WordLength = UART_WORDLENGTH_8B;
    g_uart1.Init.StopBits = UART_STOPBITS_1;
    g_uart1.Init.Parity = UART_PARITY_NONE;
    g_uart1.Init.Mode = UART_MODE_TX_RX;
    g_uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_uart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&g_uart1) != HAL_OK)
    {
        for (;;)
        {
            __NOP();
        }
    }

    __HAL_LINKDMA(&g_uart1, hdmarx, g_dma_uart1_rx);
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 1U, 0U);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    HAL_NVIC_SetPriority(USART1_IRQn, 1U, 0U);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

static uint16_t crc16_modbus(const uint8_t *data, uint8_t length)
{
    uint16_t crc = 0xFFFFU;
    uint8_t index;

    for (index = 0U; index < length; ++index)
    {
        uint8_t bit;
        crc ^= data[index];
        for (bit = 0U; bit < 8U; ++bit)
        {
            crc = ((crc & 1U) != 0U) ?
                  (uint16_t)((crc >> 1U) ^ 0xA001U) :
                  (uint16_t)(crc >> 1U);
        }
    }
    return crc;
}

static void process_received_byte(uint8_t value)
{
    if ((g_request_length == 0U) && (value != 0x55U))
    {
        return;
    }
    if ((g_request_length == 1U) && (value != 0xAAU))
    {
        g_request_length = (value == 0x55U) ? 1U : 0U;
        return;
    }

    g_request[g_request_length] = value;
    ++g_request_length;
    if (g_request_length == FRAME_SIZE)
    {
        process_request();
        g_request_length = 0U;
    }
}

static void process_request(void)
{
    const uint16_t received_crc =
        (uint16_t)g_request[8] | ((uint16_t)g_request[9] << 8U);
    uint16_t response_crc;
    uint8_t index;

    if (g_tx_busy || (g_request[2] != 0x01U) ||
        (g_request[3] != 0x04U) ||
        (crc16_modbus(g_request, 8U) != received_crc))
    {
        return;
    }

    ++g_rs485_diagnostics.valid_requests;
    g_response[0] = 0x55U;
    g_response[1] = 0xAAU;
    g_response[2] = 0x81U;
    g_response[3] = 0x04U;
    for (index = 0U; index < 4U; ++index)
    {
        g_response[4U + index] = g_request[4U + index];
    }
    response_crc = crc16_modbus(g_response, 8U);
    g_response[8] = (uint8_t)(response_crc & 0xFFU);
    g_response[9] = (uint8_t)(response_crc >> 8U);

    g_tx_busy = true;
    g_response_due_ms = HAL_GetTick() + 50U;
    g_response_pending = true;
}
