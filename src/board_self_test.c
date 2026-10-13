#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LED_PORT                 GPIOC
#define LED_PIN                  GPIO_PIN_13
#define RAM_TEST_WORDS           1024U
#define FLASH_TEST_PAGE          0x08007C00UL
#define FLASH_TEST_VALUE         0xA55A3CC3UL
#define WATCHDOG_MARKER          0x51D6U
#define RESULT_SIGNATURE         0x54455354UL
#define TIMEOUT_MS               1000U

typedef enum
{
    TEST_NONE = 0U,
    TEST_DEVICE = 1U,
    TEST_CLOCK = 2U,
    TEST_RAM = 3U,
    TEST_CRC = 4U,
    TEST_DMA = 5U,
    TEST_FLASH = 6U,
    TEST_ADC = 7U,
    TEST_TIMERS = 8U,
    TEST_CAN = 9U,
    TEST_LSE = 10U,
    TEST_RTC = 11U,
    TEST_BACKUP = 12U,
    TEST_WATCHDOG = 13U
} test_id_t;

typedef enum
{
    DEVICE_UNKNOWN = 0U,
    DEVICE_F103C6 = 6U,
    DEVICE_F103C8 = 8U
} device_kind_t;

typedef struct
{
    uint32_t signature;
    uint32_t completed_mask;
    uint32_t error_mask;
    uint32_t warning_mask;
    uint32_t device_kind;
    uint32_t device_id;
    uint32_t flash_kib;
    uint32_t adc_vref_raw;
    uint32_t adc_temp_raw;
    uint32_t watchdog_reset_seen;
} self_test_result_t;

volatile self_test_result_t g_self_test_result;

static uint32_t g_ram_test[RAM_TEST_WORDS];
static uint32_t g_dma_source[16];
static uint32_t g_dma_destination[16];
static RTC_HandleTypeDef g_rtc;

static void gpio_init(void);
static device_kind_t identify_device(void);
static bool clock_test(void);
static bool ram_test(void);
static bool crc_test(void);
static bool dma_test(void);
static bool flash_test(void);
static bool adc_test(void);
static bool timers_test(device_kind_t device);
static bool can_test(void);
static bool lse_test(void);
static bool rtc_test(bool lse_available);
static bool backup_test(void);
static bool watchdog_reset_seen(void);
static void start_watchdog_reset(void);
static void mark_passed(test_id_t test);
static void mark_failed(test_id_t test);
static void mark_warning(test_id_t test);
static void signal_forever(void);
static bool timed_out(uint32_t start, uint32_t timeout);

void SysTick_Handler(void)
{
    HAL_IncTick();
}

int main(void)
{
    const bool watchdog_passed = watchdog_reset_seen();
    device_kind_t device;
    bool lse_available;
    bool backup_passed;

    HAL_Init();
    gpio_init();

    g_self_test_result.signature = RESULT_SIGNATURE;
    g_self_test_result.completed_mask = 0U;
    g_self_test_result.error_mask = 0U;
    g_self_test_result.warning_mask = 0U;
    g_self_test_result.watchdog_reset_seen = watchdog_passed ? 1U : 0U;

    device = identify_device();
    if (device == DEVICE_UNKNOWN)
    {
        mark_failed(TEST_DEVICE);
    }
    if (!clock_test())
    {
        mark_failed(TEST_CLOCK);
    }
    if (!ram_test())
    {
        mark_failed(TEST_RAM);
    }
    if (!crc_test())
    {
        mark_failed(TEST_CRC);
    }
    if (!dma_test())
    {
        mark_failed(TEST_DMA);
    }
    if (!flash_test())
    {
        mark_failed(TEST_FLASH);
    }
    if (!adc_test())
    {
        mark_failed(TEST_ADC);
    }
    if (!timers_test(device))
    {
        mark_failed(TEST_TIMERS);
    }
    if (!can_test())
    {
        mark_failed(TEST_CAN);
    }

    lse_available = lse_test();
    if (!lse_available)
    {
        mark_warning(TEST_LSE);
    }
    if (!rtc_test(lse_available))
    {
        mark_failed(TEST_RTC);
    }

    backup_passed = backup_test();
    if (!backup_passed)
    {
        mark_failed(TEST_BACKUP);
    }

    if (watchdog_passed)
    {
        mark_passed(TEST_WATCHDOG);
    }
    else if (backup_passed)
    {
        start_watchdog_reset();
    }
    else
    {
        mark_failed(TEST_WATCHDOG);
    }

    signal_forever();
}

static void gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
}

static device_kind_t identify_device(void)
{
    const uint32_t device_id = HAL_GetDEVID();
    uint16_t flash_kib;
    device_kind_t device = DEVICE_UNKNOWN;

    /* REGMAP: factory flash-size register in STM32F1 system memory. */
    flash_kib = *((volatile const uint16_t *)FLASHSIZE_BASE);
    g_self_test_result.device_id = device_id;
    g_self_test_result.flash_kib = flash_kib;

    /*
     * DBGMCU_IDCODE is not reliable outside an active debug session on F103.
     * The factory flash-size value remains readable after a standalone reset.
     */
    if (flash_kib == 32U)
    {
        device = DEVICE_F103C6;
    }
    else if (flash_kib >= 64U)
    {
        device = DEVICE_F103C8;
    }

    g_self_test_result.device_kind = (uint32_t)device;
    if (device != DEVICE_UNKNOWN)
    {
        mark_passed(TEST_DEVICE);
    }
    return device;
}

static bool clock_test(void)
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
        return false;
    }

    clocks.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clocks.APB1CLKDivider = RCC_HCLK_DIV2;
    clocks.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_2) != HAL_OK)
    {
        return false;
    }

    if ((HAL_RCC_GetSysClockFreq() != 72000000U) ||
        (HAL_RCC_GetPCLK1Freq() != 36000000U) ||
        (HAL_RCC_GetPCLK2Freq() != 72000000U))
    {
        return false;
    }

    mark_passed(TEST_CLOCK);
    return true;
}

static bool ram_test(void)
{
    uint32_t index;

    for (index = 0U; index < RAM_TEST_WORDS; ++index)
    {
        g_ram_test[index] = 0xAAAAAAAAUL;
    }
    for (index = 0U; index < RAM_TEST_WORDS; ++index)
    {
        if (g_ram_test[index] != 0xAAAAAAAAUL)
        {
            return false;
        }
        g_ram_test[index] = 0x55555555UL;
    }
    for (index = RAM_TEST_WORDS; index > 0U; --index)
    {
        if (g_ram_test[index - 1U] != 0x55555555UL)
        {
            return false;
        }
        g_ram_test[index - 1U] = 0U;
    }

    mark_passed(TEST_RAM);
    return true;
}

static uint32_t software_crc32(const uint32_t *data, size_t count)
{
    uint32_t crc = 0xFFFFFFFFUL;
    size_t index;

    for (index = 0U; index < count; ++index)
    {
        uint32_t bit;
        crc ^= data[index];
        for (bit = 0U; bit < 32U; ++bit)
        {
            crc = ((crc & 0x80000000UL) != 0U) ?
                  ((crc << 1U) ^ 0x04C11DB7UL) : (crc << 1U);
        }
    }
    return crc;
}

static bool crc_test(void)
{
    CRC_HandleTypeDef crc = {0};
    uint32_t data[4] = {
        0x12345678UL, 0xA5A5A5A5UL, 0x00000000UL, 0xFFFFFFFFUL
    };
    uint32_t hardware_result;

    __HAL_RCC_CRC_CLK_ENABLE();
    crc.Instance = CRC;
    if (HAL_CRC_Init(&crc) != HAL_OK)
    {
        return false;
    }
    hardware_result = HAL_CRC_Calculate(&crc, data, 4U);
    (void)HAL_CRC_DeInit(&crc);
    __HAL_RCC_CRC_CLK_DISABLE();

    if (hardware_result != software_crc32(data, 4U))
    {
        return false;
    }
    mark_passed(TEST_CRC);
    return true;
}

static bool dma_test(void)
{
    DMA_HandleTypeDef dma = {0};
    uint32_t index;
    bool passed = true;

    for (index = 0U; index < 16U; ++index)
    {
        g_dma_source[index] = 0x13570000UL + index;
        g_dma_destination[index] = 0U;
    }

    __HAL_RCC_DMA1_CLK_ENABLE();
    dma.Instance = DMA1_Channel1;
    dma.Init.Direction = DMA_MEMORY_TO_MEMORY;
    dma.Init.PeriphInc = DMA_PINC_ENABLE;
    dma.Init.MemInc = DMA_MINC_ENABLE;
    dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    dma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    dma.Init.Mode = DMA_NORMAL;
    dma.Init.Priority = DMA_PRIORITY_HIGH;

    if ((HAL_DMA_Init(&dma) != HAL_OK) ||
        (HAL_DMA_Start(&dma, (uint32_t)g_dma_source,
                       (uint32_t)g_dma_destination, 16U) != HAL_OK) ||
        (HAL_DMA_PollForTransfer(&dma, HAL_DMA_FULL_TRANSFER,
                                 TIMEOUT_MS) != HAL_OK))
    {
        passed = false;
    }

    for (index = 0U; passed && (index < 16U); ++index)
    {
        if (g_dma_destination[index] != g_dma_source[index])
        {
            passed = false;
        }
    }

    (void)HAL_DMA_DeInit(&dma);
    __HAL_RCC_DMA1_CLK_DISABLE();
    if (!passed)
    {
        return false;
    }
    mark_passed(TEST_DMA);
    return true;
}

static bool flash_page_erased(void)
{
    uint32_t offset;

    for (offset = 0U; offset < FLASH_PAGE_SIZE; offset += sizeof(uint32_t))
    {
        /* REGMAP: reserved final page in the 32-KiB C6 address range. */
        const volatile uint32_t *word =
            (const volatile uint32_t *)(FLASH_TEST_PAGE + offset);
        if (*word != 0xFFFFFFFFUL)
        {
            return false;
        }
    }
    return true;
}

static bool erase_test_page(void)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = FLASH_TEST_PAGE;
    erase.NbPages = 1U;
    return HAL_FLASHEx_Erase(&erase, &page_error) == HAL_OK;
}

static bool flash_test(void)
{
    bool passed = false;

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return false;
    }

    if (erase_test_page() && flash_page_erased() &&
        (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_TEST_PAGE,
                           FLASH_TEST_VALUE) == HAL_OK))
    {
        /* REGMAP: read back the reserved diagnostic flash word. */
        const volatile uint32_t *word =
            (const volatile uint32_t *)FLASH_TEST_PAGE;
        passed = (*word == FLASH_TEST_VALUE);
    }

    if (!erase_test_page() || !flash_page_erased())
    {
        passed = false;
    }
    (void)HAL_FLASH_Lock();

    if (!passed)
    {
        return false;
    }
    mark_passed(TEST_FLASH);
    return true;
}

static bool adc_read(ADC_HandleTypeDef *adc, uint32_t channel,
                     uint32_t *value)
{
    ADC_ChannelConfTypeDef configuration = {0};

    configuration.Channel = channel;
    configuration.Rank = ADC_REGULAR_RANK_1;
    configuration.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if ((HAL_ADC_ConfigChannel(adc, &configuration) != HAL_OK) ||
        (HAL_ADC_Start(adc) != HAL_OK) ||
        (HAL_ADC_PollForConversion(adc, TIMEOUT_MS) != HAL_OK))
    {
        return false;
    }
    *value = HAL_ADC_GetValue(adc);
    return HAL_ADC_Stop(adc) == HAL_OK;
}

static bool adc_test(void)
{
    RCC_PeriphCLKInitTypeDef clock = {0};
    ADC_HandleTypeDef adc = {0};
    uint32_t vref = 0U;
    uint32_t temperature = 0U;
    bool passed;

    clock.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    clock.AdcClockSelection = RCC_ADCPCLK2_DIV6;
    if (HAL_RCCEx_PeriphCLKConfig(&clock) != HAL_OK)
    {
        return false;
    }

    __HAL_RCC_ADC1_CLK_ENABLE();
    adc.Instance = ADC1;
    adc.Init.ScanConvMode = ADC_SCAN_DISABLE;
    adc.Init.ContinuousConvMode = DISABLE;
    adc.Init.DiscontinuousConvMode = DISABLE;
    adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc.Init.NbrOfConversion = 1U;

    passed = (HAL_ADC_Init(&adc) == HAL_OK) &&
             (HAL_ADCEx_Calibration_Start(&adc) == HAL_OK) &&
             adc_read(&adc, ADC_CHANNEL_VREFINT, &vref) &&
             adc_read(&adc, ADC_CHANNEL_TEMPSENSOR, &temperature);

    (void)HAL_ADC_DeInit(&adc);
    __HAL_RCC_ADC1_CLK_DISABLE();
    g_self_test_result.adc_vref_raw = vref;
    g_self_test_result.adc_temp_raw = temperature;

    if (!passed || (vref < 1000U) || (vref > 2500U) ||
        (temperature < 500U) || (temperature > 2500U))
    {
        return false;
    }
    mark_passed(TEST_ADC);
    return true;
}

static bool one_timer_test(TIM_TypeDef *instance)
{
    TIM_HandleTypeDef timer = {0};
    uint32_t start;
    uint32_t initial;
    bool passed;

    timer.Instance = instance;
    timer.Init.Prescaler = 71U;
    timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer.Init.Period = 0xFFFFU;
    timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if ((HAL_TIM_Base_Init(&timer) != HAL_OK) ||
        (HAL_TIM_Base_Start(&timer) != HAL_OK))
    {
        return false;
    }

    initial = __HAL_TIM_GET_COUNTER(&timer);
    start = HAL_GetTick();
    while (!timed_out(start, 2U))
    {
        __NOP();
    }
    passed = (__HAL_TIM_GET_COUNTER(&timer) != initial);
    (void)HAL_TIM_Base_Stop(&timer);
    (void)HAL_TIM_Base_DeInit(&timer);
    return passed;
}

static bool timers_test(device_kind_t device)
{
    bool passed;

    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    passed = one_timer_test(TIM1) &&
             one_timer_test(TIM2) &&
             one_timer_test(TIM3);

#if defined(STM32F103xB)
    if (passed && (device == DEVICE_F103C8))
    {
        __HAL_RCC_TIM4_CLK_ENABLE();
        passed = one_timer_test(TIM4);
        __HAL_RCC_TIM4_CLK_DISABLE();
    }
#else
    (void)device;
#endif

    __HAL_RCC_TIM1_CLK_DISABLE();
    __HAL_RCC_TIM2_CLK_DISABLE();
    __HAL_RCC_TIM3_CLK_DISABLE();
    if (!passed)
    {
        return false;
    }
    mark_passed(TEST_TIMERS);
    return true;
}

static bool can_test(void)
{
    CAN_HandleTypeDef can = {0};
    CAN_FilterTypeDef filter = {0};
    CAN_TxHeaderTypeDef tx_header = {0};
    CAN_RxHeaderTypeDef rx_header = {0};
    uint8_t tx_data[8] = {0x42U, 0x10U, 0xA5U, 0x5AU, 1U, 2U, 3U, 4U};
    uint8_t rx_data[8] = {0};
    uint32_t mailbox = 0U;
    uint32_t start;
    uint32_t index;
    bool passed = true;

    __HAL_RCC_CAN1_CLK_ENABLE();
    can.Instance = CAN1;
    can.Init.Prescaler = 4U;
    can.Init.Mode = CAN_MODE_LOOPBACK;
    can.Init.SyncJumpWidth = CAN_SJW_1TQ;
    can.Init.TimeSeg1 = CAN_BS1_8TQ;
    can.Init.TimeSeg2 = CAN_BS2_3TQ;
    can.Init.TimeTriggeredMode = DISABLE;
    can.Init.AutoBusOff = DISABLE;
    can.Init.AutoWakeUp = DISABLE;
    can.Init.AutoRetransmission = ENABLE;
    can.Init.ReceiveFifoLocked = DISABLE;
    can.Init.TransmitFifoPriority = DISABLE;

    filter.FilterBank = 0U;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0U;
    filter.FilterIdLow = 0U;
    filter.FilterMaskIdHigh = 0U;
    filter.FilterMaskIdLow = 0U;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14U;

    tx_header.StdId = 0x321U;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = 8U;
    tx_header.TransmitGlobalTime = DISABLE;

    if ((HAL_CAN_Init(&can) != HAL_OK) ||
        (HAL_CAN_ConfigFilter(&can, &filter) != HAL_OK) ||
        (HAL_CAN_Start(&can) != HAL_OK) ||
        (HAL_CAN_AddTxMessage(&can, &tx_header, tx_data, &mailbox) != HAL_OK))
    {
        passed = false;
    }

    start = HAL_GetTick();
    while (passed &&
           (HAL_CAN_GetRxFifoFillLevel(&can, CAN_RX_FIFO0) == 0U) &&
           !timed_out(start, 100U))
    {
        __NOP();
    }

    if (passed &&
        ((HAL_CAN_GetRxFifoFillLevel(&can, CAN_RX_FIFO0) == 0U) ||
         (HAL_CAN_GetRxMessage(&can, CAN_RX_FIFO0,
                               &rx_header, rx_data) != HAL_OK)))
    {
        passed = false;
    }
    if (passed && ((rx_header.StdId != tx_header.StdId) ||
                   (rx_header.DLC != tx_header.DLC)))
    {
        passed = false;
    }
    for (index = 0U; passed && (index < 8U); ++index)
    {
        if (rx_data[index] != tx_data[index])
        {
            passed = false;
        }
    }

    (void)HAL_CAN_Stop(&can);
    (void)HAL_CAN_DeInit(&can);
    __HAL_RCC_CAN1_CLK_DISABLE();
    if (!passed)
    {
        return false;
    }
    mark_passed(TEST_CAN);
    return true;
}

static bool lse_test(void)
{
    uint32_t start;

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    __HAL_RCC_LSE_CONFIG(RCC_LSE_ON);
    start = HAL_GetTick();
    while ((__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET) &&
           !timed_out(start, 5000U))
    {
        __NOP();
    }

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET)
    {
        __HAL_RCC_LSE_CONFIG(RCC_LSE_OFF);
        return false;
    }

    mark_passed(TEST_LSE);
    return true;
}

static bool rtc_test(bool lse_available)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_PeriphCLKInitTypeDef clock = {0};
    RTC_TimeTypeDef before = {0};
    RTC_TimeTypeDef after = {0};
    uint32_t start;

    if (!lse_available)
    {
        oscillator.OscillatorType = RCC_OSCILLATORTYPE_LSI;
        oscillator.LSIState = RCC_LSI_ON;
        if (HAL_RCC_OscConfig(&oscillator) != HAL_OK)
        {
            return false;
        }
    }

    clock.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    clock.RTCClockSelection = lse_available ?
                             RCC_RTCCLKSOURCE_LSE : RCC_RTCCLKSOURCE_LSI;
    if (HAL_RCCEx_PeriphCLKConfig(&clock) != HAL_OK)
    {
        return false;
    }

    __HAL_RCC_RTC_ENABLE();
    g_rtc.Instance = RTC;
    g_rtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    g_rtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
    if (HAL_RTC_Init(&g_rtc) != HAL_OK)
    {
        return false;
    }

    if (HAL_RTC_GetTime(&g_rtc, &before, RTC_FORMAT_BIN) != HAL_OK)
    {
        return false;
    }

    start = HAL_GetTick();
    do
    {
        if (HAL_RTC_GetTime(&g_rtc, &after, RTC_FORMAT_BIN) != HAL_OK)
        {
            return false;
        }
    }
    while ((after.Seconds == before.Seconds) && !timed_out(start, 2500U));

    if (after.Seconds == before.Seconds)
    {
        return false;
    }
    mark_passed(TEST_RTC);
    return true;
}

static bool backup_test(void)
{
    HAL_RTCEx_BKUPWrite(&g_rtc, RTC_BKP_DR2, 0x5A3CU);
    if (HAL_RTCEx_BKUPRead(&g_rtc, RTC_BKP_DR2) != 0x5A3CU)
    {
        return false;
    }

    mark_passed(TEST_BACKUP);
    return true;
}

static bool watchdog_reset_seen(void)
{
    RTC_HandleTypeDef rtc = {0};
    bool passed;

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    rtc.Instance = RTC;

    passed = (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET) &&
             (HAL_RTCEx_BKUPRead(&rtc, RTC_BKP_DR1) == WATCHDOG_MARKER);
    if (passed)
    {
        HAL_RTCEx_BKUPWrite(&rtc, RTC_BKP_DR1, 0U);
    }
    __HAL_RCC_CLEAR_RESET_FLAGS();
    return passed;
}

static void start_watchdog_reset(void)
{
    IWDG_HandleTypeDef watchdog = {0};

    HAL_RTCEx_BKUPWrite(&g_rtc, RTC_BKP_DR1, WATCHDOG_MARKER);
    watchdog.Instance = IWDG;
    watchdog.Init.Prescaler = IWDG_PRESCALER_64;
    watchdog.Init.Reload = 50U;
    (void)HAL_IWDG_Init(&watchdog);
    for (;;)
    {
        __NOP();
    }
}

static void mark_passed(test_id_t test)
{
    g_self_test_result.completed_mask |= (1UL << (uint32_t)test);
    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
}

static void mark_failed(test_id_t test)
{
    g_self_test_result.completed_mask |= (1UL << (uint32_t)test);
    g_self_test_result.error_mask |= (1UL << (uint32_t)test);
}

static void mark_warning(test_id_t test)
{
    g_self_test_result.completed_mask |= (1UL << (uint32_t)test);
    g_self_test_result.warning_mask |= (1UL << (uint32_t)test);
}

static void signal_forever(void)
{
    uint32_t error_count = 0U;
    uint32_t bit;

    for (bit = 1U; bit <= (uint32_t)TEST_WATCHDOG; ++bit)
    {
        if ((g_self_test_result.error_mask & (1UL << bit)) != 0U)
        {
            ++error_count;
        }
    }

    for (;;)
    {
        const uint32_t cycle = HAL_GetTick() % 3000U;
        bool enabled;

        if (error_count != 0U)
        {
            const uint32_t pulse_area = error_count * 200U;
            enabled = (cycle < pulse_area) && ((cycle % 200U) < 80U);
        }
        else if (g_self_test_result.warning_mask != 0U)
        {
            enabled = (cycle < 100U) ||
                      ((cycle >= 250U) && (cycle < 350U)) ||
                      ((cycle >= 500U) && (cycle < 600U));
        }
        else
        {
            enabled = (cycle < 100U) ||
                      ((cycle >= 250U) && (cycle < 350U));
        }

        HAL_GPIO_WritePin(LED_PORT, LED_PIN,
                          enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
}

static bool timed_out(uint32_t start, uint32_t timeout)
{
    return (HAL_GetTick() - start) >= timeout;
}
