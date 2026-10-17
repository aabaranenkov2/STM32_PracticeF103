#include "stm32f1xx_hal.h"
#include "ground_sensor_pins.h"
#include "lot_sensor_protocol.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define RS485_DE_PORT              GPIOB
#define RS485_DE_PIN               GPIO_PIN_0
#define MOT_DIR_WHITE_PORT         GPIOB
#define MOT_DIR_WHITE_PIN          GPIO_PIN_15
#define MOT_FG_YELLOW_PORT         GPIOB
#define MOT_FG_YELLOW_PIN          GPIO_PIN_14
#define MOT_PWM_BLUE_PORT          GPIOA
#define MOT_PWM_BLUE_PIN           GPIO_PIN_8
#define ENC_FA_WHITE_PORT          GPIOB
#define ENC_FA_WHITE_PIN           GPIO_PIN_4
#define ENC_FB_GREEN_PORT          GPIOB
#define ENC_FB_GREEN_PIN           GPIO_PIN_5
#define STATUS_LED_PORT            GPIOC
#define STATUS_LED_PIN             GPIO_PIN_13
#define STATE_LED_PORT             GPIOB
#define STATE_LED_PIN              GPIO_PIN_1
#define STATE_LED_BUSY_HALF_MS     500U
#define STATE_LED_FAULT_HALF_MS    125U
#define INTERLOCK_INPUT_PORT       GPIOB
#define INTERLOCK_INPUT_PIN        GPIO_PIN_10
#define BUSY_OUTPUT_PORT           GPIOB
#define BUSY_OUTPUT_PIN            GPIO_PIN_11

#define DMA_RX_SIZE                32U
#define RX_RING_SIZE               128U
#define FRAME_MAX_SIZE             (LOT_PROTOCOL_MAX_PAYLOAD + 8U)
#define BUTTON_COUNT               5U
#define BUTTON_DEBOUNCE_TICKS      3U
#define APP_TICK_MS                10U
#define SPEED_SAMPLE_MS            APP_TICK_MS
#define TX_TURNAROUND_MS           10U
#define CONFIG_MAGIC               0x4C4F5431UL
#define CONFIG_VERSION             15U
#define LENGTH_CAL_SCALE           10000UL
#define LENGTH_CAL_MIN             1000UL
#define LENGTH_CAL_MAX             200000UL
#define LENGTH_CAL_OFFSET_MIN      (-500000L)
#define LENGTH_CAL_OFFSET_MAX      500000L
#define NOMINAL_SHAFT_DIAMETER_MM  10U
#define MEASURE_COUNTS_PER_REV     2400UL
#define MEASURE_CIRCUMFERENCE_UM   31416UL
#define MOTOR_PULSES_PER_REV       9UL
#define MOTOR_CIRCUMFERENCE_UM     125664UL
#define MOTOR_GEAR_RATIO_X10       188U
#define MOTOR_MIN_ACTIVE_PWM       450U
#define PARK_FG_MOVEMENT_PULSES    20U
#define CONTACT_REVERSAL_PAUSE_MS  150UL
#define MEASURE_REVERSAL_PAUSE_MS  20UL
#define TAKEUP_TIMEOUT_MS          15000UL
#define TAKEUP_CONFIRM_TENTH_MM    10L
#define ASCENT_RAMP_TENTH_MM       500L
#define LOCAL_AUTO_INTERVAL_MS     3000UL
#define DESCENT_START_RETRY_MS     1000UL
#define DESCENT_START_MAX_RETRIES  3U
#define PARK_RELEASE_MIN_MS        1000UL
#define PARK_TEST_START_RETRY_MS   1000UL
#define PARK_TEST_MAX_RETRIES      3U

#if defined(STM32F103x6)
#define DEVICE_FLASH_SIZE          (32UL * 1024UL)
#else
#define DEVICE_FLASH_SIZE          (64UL * 1024UL)
#endif
#define CONFIG_FLASH_ADDRESS       (FLASH_BASE + DEVICE_FLASH_SIZE - FLASH_PAGE_SIZE)

typedef struct
{
    uint16_t run_pwm;
    uint16_t park_pwm;
    uint16_t contact_timeout_ms;
    uint16_t stall_timeout_ms;
    uint16_t mismatch_mm;
    uint16_t park_release_ms;
    uint16_t park_test_ms;
    uint16_t park_test_pulses;
    uint16_t contact_speed_percent;
    uint16_t slowdown_remaining_percent;
    uint16_t slowdown_divisor_tenths;
    uint16_t manual_timeout_ms;
    uint32_t motion_timeout_ms;
    uint32_t auto_interval_ms;
    uint32_t length_cal_x10000;
    uint32_t motor_cal_x10000;
    int32_t length_cal_offset_tenth_mm;
    uint16_t contact_ignore_ms;
    uint16_t brake_comp_mm;
    uint16_t load_height_mm;
    uint16_t motor_encoder_enabled;
    int16_t descent_correction_mm;
    uint16_t error_measurement_limit;
    uint32_t max_descent_mm;
    uint16_t takeup_pwm;
    int16_t ascent_correction_mm;
} lot_config_t;

typedef struct
{
    uint16_t run_pwm;
    uint16_t park_pwm;
    uint16_t contact_timeout_ms;
    uint16_t stall_timeout_ms;
    uint16_t mismatch_mm;
    uint16_t park_release_ms;
    uint16_t park_test_ms;
    uint16_t park_test_pulses;
    uint16_t contact_speed_percent;
    uint16_t slowdown_remaining_percent;
    uint16_t slowdown_divisor_tenths;
    uint16_t manual_timeout_ms;
    uint32_t motion_timeout_ms;
    uint32_t auto_interval_ms;
    uint32_t length_cal_x10000;
    uint32_t motor_cal_x10000;
    int32_t length_cal_offset_tenth_mm;
    uint16_t contact_ignore_ms;
    uint16_t brake_comp_mm;
    uint16_t load_height_mm;
    uint16_t motor_encoder_enabled;
    int16_t descent_correction_mm;
    uint16_t error_measurement_limit;
    uint32_t max_descent_mm;
} lot_config_v14_t;

typedef struct
{
    uint16_t run_pwm;
    uint16_t park_pwm;
    uint16_t contact_timeout_ms;
    uint16_t stall_timeout_ms;
    uint16_t mismatch_mm;
    uint16_t park_release_ms;
    uint16_t park_test_ms;
    uint16_t park_test_pulses;
    uint16_t contact_speed_percent;
    uint16_t slowdown_remaining_percent;
    uint16_t slowdown_divisor_tenths;
    uint16_t manual_timeout_ms;
    uint32_t motion_timeout_ms;
    uint32_t auto_interval_ms;
    uint32_t length_cal_x10000;
    uint32_t motor_cal_x10000;
    int32_t length_cal_offset_tenth_mm;
    uint16_t contact_ignore_ms;
    uint16_t brake_comp_mm;
    uint16_t load_height_mm;
    uint16_t motor_encoder_enabled;
    int16_t descent_correction_mm;
    uint16_t error_measurement_limit;
} lot_config_v13_t;

typedef struct
{
    uint16_t run_pwm;
    uint16_t park_pwm;
    uint16_t contact_timeout_ms;
    uint16_t stall_timeout_ms;
    uint16_t mismatch_mm;
    uint16_t park_release_ms;
    uint16_t park_test_ms;
    uint16_t park_test_pulses;
    uint16_t contact_speed_percent;
    uint16_t slowdown_remaining_percent;
    uint16_t slowdown_divisor_tenths;
    uint16_t manual_timeout_ms;
    uint32_t motion_timeout_ms;
    uint32_t auto_interval_ms;
    uint32_t length_cal_x10000;
    uint32_t motor_cal_x10000;
    int32_t length_cal_offset_tenth_mm;
    uint16_t contact_ignore_ms;
    uint16_t brake_comp_mm;
    uint16_t load_height_mm;
    uint16_t motor_encoder_enabled;
    int16_t descent_correction_mm;
} lot_config_v12_t;

typedef struct
{
    uint16_t max_fill_x10;
    uint16_t target_fill_x10;
    uint32_t model_stroke_mm;
    uint32_t upper_height_mm;
    uint32_t upper_top_diameter_mm;
    uint32_t bunker_diameter_mm;
    uint32_t cylinder_height_mm;
    uint32_t lower_height_mm;
    uint32_t lower_bottom_diameter_mm;
    uint32_t sensor_height_above_base_mm;
    uint32_t density_x1000;
    uint32_t product_factor_x1000;
} bunker_config_t;

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    lot_config_t config;
    bunker_config_t bunker;
    uint16_t crc;
} config_record_t;

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    lot_config_v14_t config;
    bunker_config_t bunker;
    uint16_t crc;
} config_record_v14_t;

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    lot_config_v13_t config;
    bunker_config_t bunker;
    uint16_t crc;
} config_record_v13_t;

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    lot_config_v12_t config;
    bunker_config_t bunker;
    uint16_t crc;
} config_record_v12_t;

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    lot_config_v12_t config;
    uint16_t crc;
} config_record_v11_t;

typedef struct
{
    uint16_t run_pwm;
    uint16_t park_pwm;
    uint16_t contact_timeout_ms;
    uint16_t stall_timeout_ms;
    uint16_t mismatch_mm;
    uint16_t park_release_ms;
    uint16_t park_test_ms;
    uint16_t park_test_pulses;
    uint16_t contact_speed_percent;
    uint16_t slowdown_remaining_percent;
    uint16_t slowdown_divisor_tenths;
    uint16_t manual_timeout_ms;
    uint32_t motion_timeout_ms;
    uint32_t auto_interval_ms;
    uint32_t length_cal_x10000;
    uint32_t motor_cal_x10000;
    int32_t length_cal_offset_tenth_mm;
} lot_config_v7_t;

typedef struct
{
    uint16_t run_pwm;
    uint16_t park_pwm;
    uint16_t contact_timeout_ms;
    uint16_t stall_timeout_ms;
    uint16_t mismatch_mm;
    uint16_t park_release_ms;
    uint16_t park_test_ms;
    uint16_t park_test_pulses;
    uint16_t contact_speed_percent;
    uint16_t slowdown_remaining_percent;
    uint16_t slowdown_divisor_tenths;
    uint16_t manual_timeout_ms;
    uint32_t motion_timeout_ms;
    uint32_t auto_interval_ms;
    uint32_t length_cal_x10000;
    uint32_t motor_cal_x10000;
    int32_t length_cal_offset_tenth_mm;
    uint16_t contact_ignore_ms;
} lot_config_v8_t;

typedef struct
{
    uint16_t run_pwm;
    uint16_t park_pwm;
    uint16_t contact_timeout_ms;
    uint16_t stall_timeout_ms;
    uint16_t mismatch_mm;
    uint16_t park_release_ms;
    uint16_t park_test_ms;
    uint16_t park_test_pulses;
    uint16_t contact_speed_percent;
    uint16_t slowdown_remaining_percent;
    uint16_t slowdown_divisor_tenths;
    uint16_t manual_timeout_ms;
    uint32_t motion_timeout_ms;
    uint32_t auto_interval_ms;
    uint32_t length_cal_x10000;
    uint32_t motor_cal_x10000;
    int32_t length_cal_offset_tenth_mm;
    uint16_t contact_ignore_ms;
    uint16_t brake_comp_mm;
    uint16_t load_height_mm;
} lot_config_v9_t;

typedef struct
{
    uint16_t run_pwm;
    uint16_t park_pwm;
    uint16_t contact_timeout_ms;
    uint16_t stall_timeout_ms;
    uint16_t mismatch_mm;
    uint16_t park_release_ms;
    uint16_t park_test_ms;
    uint16_t park_test_pulses;
    uint16_t contact_speed_percent;
    uint16_t slowdown_remaining_percent;
    uint16_t slowdown_divisor_tenths;
    uint16_t manual_timeout_ms;
    uint32_t motion_timeout_ms;
    uint32_t auto_interval_ms;
    uint32_t length_cal_x10000;
    uint32_t motor_cal_x10000;
    int32_t length_cal_offset_tenth_mm;
    uint16_t contact_ignore_ms;
    uint16_t brake_comp_mm;
    uint16_t load_height_mm;
    uint16_t motor_encoder_enabled;
} lot_config_v10_t;

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    lot_config_v8_t config;
    uint16_t crc;
} config_record_v8_t;

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    lot_config_v9_t config;
    uint16_t crc;
} config_record_v9_t;

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    lot_config_v10_t config;
    uint16_t crc;
} config_record_v10_t;

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    lot_config_v7_t config;
    uint16_t crc;
} config_record_v7_t;

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t version;
    uint8_t config_data[40U];
    uint16_t crc;
} config_record_v5_t;

typedef struct
{
    lot_state_t state;
    uint16_t errors;
    bool auto_enabled;
    bool motor_enabled;
    bool motor_up;
    uint16_t pwm;
    int32_t encoder_position;
    int32_t descent_distance;
    int32_t descent_fg_count;
    int32_t last_distance;
    int32_t ascent_distance;
    int32_t travel_difference;
    int32_t fg_count;
    uint32_t measurement_count;
    uint32_t encoder_speed_cps;
} lot_status_t;

static UART_HandleTypeDef g_uart1;
static DMA_HandleTypeDef g_dma_uart1_rx;
static TIM_HandleTypeDef g_tim1_pwm;
static TIM_HandleTypeDef g_tim3_encoder;
static IWDG_HandleTypeDef g_iwdg;

static volatile uint8_t g_dma_rx[DMA_RX_SIZE];
static volatile uint8_t g_rx_ring[RX_RING_SIZE];
static volatile uint16_t g_rx_write;
static volatile uint16_t g_rx_read;
static volatile int32_t g_fg_count;
static volatile int8_t g_fg_step_sign = 1;
static volatile uint32_t g_fg_last_ms;
static volatile bool g_tx_busy;

static uint8_t g_parser[FRAME_MAX_SIZE];
static uint8_t g_parser_length;
static uint8_t g_parser_expected;
static uint8_t g_tx_frame[FRAME_MAX_SIZE];
static uint8_t g_tx_length;
static bool g_tx_pending;
static uint32_t g_tx_due_ms;
static bool g_last_response_valid;
static uint8_t g_last_request_command;
static uint8_t g_last_request_sequence;
static uint8_t g_last_response[LOT_PROTOCOL_MAX_PAYLOAD];
static uint8_t g_last_response_length;

static lot_config_t g_config = {
    550U, 450U, 450U, 700U, 100U, 3000U, 3000U, 4U,
    80U, 10U, 20U, 10000U, 360000UL, 60000UL, LENGTH_CAL_SCALE, LENGTH_CAL_SCALE,
    0L, 2000U, 0U, 110U, 0U, 0, 2U, 1000UL, 945U, 0
};
static bunker_config_t g_bunker_config = {
    889U, 678U, 570UL, 1500UL, 300UL, 5600UL,
    10000UL, 2000UL, 1000UL, 200UL, 750UL, 1000UL
};
static lot_status_t g_status;
static uint16_t g_manual_pwm;
static uint32_t g_state_started_ms;
static uint32_t g_last_encoder_ms;
static uint32_t g_last_app_ms;
static uint32_t g_next_auto_ms;
static bool g_local_auto_enabled;
static int32_t g_state_encoder_start;
static int32_t g_state_fg_start;
static int32_t g_down_sign;
static int32_t g_measure_origin;
static int32_t g_live_down_counts;
static int32_t g_live_up_counts;
static int32_t g_indicator_depth;
static int32_t g_indicator_climb;
static int32_t g_saved_down_counts;
static int32_t g_saved_up_counts;
static int32_t g_saved_down_fg_counts;
static int32_t g_saved_up_fg_counts;
static int32_t g_contact_fg_count;
static int32_t g_saved_descent_tenth_mm;
static int32_t g_saved_ascent_tenth_mm;
static int32_t g_path_difference_tenth_mm;
static int32_t g_result_tenth_mm;
static uint8_t g_calibration_capture_flags;
static bool g_live_view_active;
static uint16_t g_encoder_previous;
static bool g_state_encoder_seen;
static bool g_state_up_seen;
static bool g_measurement_requested;
static bool g_skip_post_measure_park;
static uint16_t g_encoder_mismatch_count;
static uint8_t g_descent_start_retries;
static uint8_t g_park_test_start_retries;
static bool g_park_test_retry_waiting;
static uint32_t g_park_test_retry_started_ms;
static bool g_diagnostics_cycle_active;
static bool g_diagnostics_descent_started;
static bool g_diagnostics_ascent_started;
static uint32_t g_diagnostics_cycle_started_ms;
static uint32_t g_diagnostics_descent_started_ms;
static uint32_t g_diagnostics_ascent_started_ms;
static uint32_t g_diagnostics_sequence;
static uint32_t g_diagnostics_cycle_ms;
static uint32_t g_diagnostics_measurement_ms;
static uint32_t g_diagnostics_parking_ms;
static uint32_t g_diagnostics_descent_ms;
static uint32_t g_diagnostics_ascent_ms;
static uint16_t g_diagnostics_descent_retries;
static uint16_t g_diagnostics_park_retries;
static uint32_t g_speed_window_started_ms;
static uint32_t g_speed_window_counts;
static bool g_ground_sensor_active;
static uint8_t g_button_stable_mask;
static uint8_t g_button_candidate_mask;
static uint8_t g_button_debounce[BUTTON_COUNT];
static volatile uint8_t g_uart_glitch_pending;

static const uint16_t g_button_pins[BUTTON_COUNT] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4
};

static void motor_outputs_safe_init(void);
static void motor_pwm_pin_enable(void);
static void system_clock_config(void);
static void gpio_init(void);
static void timers_init(void);
static void uart_init(void);
static void watchdog_init(void);
static void config_load(void);
static bool config_save(void);
static bool bunker_config_values_valid(const bunker_config_t *config);
static void app_tick(uint32_t now);
static void protocol_consume(uint8_t value);
static void protocol_dispatch(const uint8_t *frame, uint8_t length);
static void protocol_send(uint8_t command, uint8_t sequence,
                          const uint8_t *payload, uint8_t length);
static void protocol_service(uint32_t now);
static uint16_t crc16_modbus(const uint8_t *data, uint8_t length);
static void calibration_capture_payload(uint8_t *payload, uint8_t *length);
static void diagnostics_payload(uint8_t *payload, uint8_t *length);
static int32_t encoder_counts_to_tenth_mm(int32_t counts);
static int32_t encoder_pulses_to_tenth_mm(int32_t counts);
static int32_t motor_pulses_to_tenth_mm(int32_t pulses);
static void live_counters_zero(uint32_t now);
static void live_view_clear(void);
static void measurement_counters_reset(uint32_t now, bool clear_result);
static bool motion_stalled_for(uint32_t now, uint32_t elapsed,
                               uint32_t idle_ms);
static void ground_sensor_reset(void);
static void ground_sensor_tick(uint32_t now);
static bool ground_sensor_descent_hit(uint32_t elapsed);
static void state_led_update(uint32_t now);
static bool interlock_input_active(void);
static bool sensor_busy(void);
static void busy_output_update(void);
static void interlock_tick(uint32_t now);
static void enter_descent_contact(uint32_t now, bool gpio_instant_takeup);
static void diagnostics_state_enter(lot_state_t state, uint32_t now);
static void diagnostics_measurement_complete(uint32_t now);
static void config_migrate_v7(const lot_config_v7_t *previous);

_Static_assert((sizeof(config_record_t) % 2U) == 0U,
               "Flash record must contain complete halfwords");

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

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(MOT_FG_YELLOW_PIN);
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == MOT_FG_YELLOW_PIN)
    {
        g_fg_count += (int32_t)g_fg_step_sign;
        g_fg_last_ms = HAL_GetTick();
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *uart, uint16_t size)
{
    uint16_t index;

    if (uart->Instance != USART1)
    {
        return;
    }

    for (index = 0U; index < size; ++index)
    {
        const uint16_t next = (uint16_t)((g_rx_write + 1U) % RX_RING_SIZE);
        if (next != g_rx_read)
        {
            g_rx_ring[g_rx_write] = g_dma_rx[index];
            g_rx_write = next;
        }
        else
        {
            g_status.errors |= LOT_ERROR_RX_OVERFLOW;
        }
    }

    (void)HAL_UARTEx_ReceiveToIdle_DMA(&g_uart1, (uint8_t *)g_dma_rx,
                                       DMA_RX_SIZE);
    __HAL_DMA_DISABLE_IT(&g_dma_uart1_rx, DMA_IT_HT);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1)
    {
        HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
        g_tx_busy = false;
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART1)
    {
        g_uart_glitch_pending = 1U;
        HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
        g_tx_busy = false;
        (void)HAL_UART_AbortReceive(&g_uart1);
        (void)HAL_UARTEx_ReceiveToIdle_DMA(&g_uart1, (uint8_t *)g_dma_rx,
                                           DMA_RX_SIZE);
        __HAL_DMA_DISABLE_IT(&g_dma_uart1_rx, DMA_IT_HT);
    }
}

static uint16_t read_u16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

static uint32_t read_u32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

static void write_u16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
}

static void write_u32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
    data[2] = (uint8_t)(value >> 16U);
    data[3] = (uint8_t)(value >> 24U);
}

static int32_t encoder_counts_to_tenth_mm(int32_t counts)
{
    int64_t distance = (int64_t)counts * (int64_t)MEASURE_CIRCUMFERENCE_UM;
    distance /= (int64_t)(MEASURE_COUNTS_PER_REV * 100UL);
    distance = distance * (int64_t)g_config.length_cal_x10000 /
        (int64_t)LENGTH_CAL_SCALE;
    distance += (int64_t)g_config.length_cal_offset_tenth_mm;

    return (int32_t)distance;
}

static int32_t encoder_pulses_to_tenth_mm(int32_t counts)
{
    int64_t distance = (int64_t)counts * (int64_t)MEASURE_CIRCUMFERENCE_UM;

    if (distance < 0)
    {
        distance = -distance;
    }
    distance /= (int64_t)(MEASURE_COUNTS_PER_REV * 100UL);
    distance = distance * (int64_t)g_config.length_cal_x10000 /
        (int64_t)LENGTH_CAL_SCALE;

    return (int32_t)distance;
}

static int32_t motor_pulses_to_tenth_mm(int32_t pulses)
{
    const bool negative = pulses < 0;
    const uint32_t magnitude =
        negative ? (uint32_t)(-pulses) : (uint32_t)pulses;
    int32_t distance = (int32_t)(
        ((uint64_t)magnitude * MOTOR_CIRCUMFERENCE_UM) /
        ((uint64_t)MOTOR_PULSES_PER_REV * MOTOR_GEAR_RATIO_X10 * 10UL));
    distance = (int32_t)(
        ((int64_t)distance * (int64_t)g_config.motor_cal_x10000) /
        (int64_t)LENGTH_CAL_SCALE);

    return negative ? -distance : distance;
}

static int32_t finish_encoder_lengths(void)
{
    int32_t down_tenth = encoder_pulses_to_tenth_mm(g_live_down_counts) +
        ((int32_t)g_config.descent_correction_mm * 10);
    const int32_t raw_up_tenth =
        encoder_pulses_to_tenth_mm(g_live_up_counts);
    int32_t up_tenth = raw_up_tenth +
        ((int32_t)g_config.ascent_correction_mm * 10);
    int32_t difference;
    int32_t up_fg_travel = g_fg_count - g_contact_fg_count;
    int32_t motor_up_tenth;
    int32_t encoder_motor_difference_tenth;
    int32_t measured_distance_tenth;
    int32_t sensor_height_tenth;

    if (up_fg_travel < 0)
    {
        up_fg_travel = -up_fg_travel;
    }
    g_saved_up_counts = g_live_up_counts;
    g_saved_up_fg_counts = up_fg_travel;
    g_calibration_capture_flags |= 0x02U;
    motor_up_tenth = motor_pulses_to_tenth_mm(up_fg_travel);
    encoder_motor_difference_tenth = motor_up_tenth - up_tenth;
    if (encoder_motor_difference_tenth < 0)
    {
        encoder_motor_difference_tenth = -encoder_motor_difference_tenth;
    }

    if (down_tenth < 0)
    {
        down_tenth = 0;
    }
    if (up_tenth < 0)
    {
        up_tenth = 0;
    }
    difference = up_tenth - down_tenth;

    if (difference < 0)
    {
        difference = -difference;
    }
    g_saved_descent_tenth_mm = down_tenth;
    g_saved_ascent_tenth_mm = up_tenth;
    g_path_difference_tenth_mm = difference;
    measured_distance_tenth =
        ((g_config.motor_encoder_enabled != 0U) ? motor_up_tenth : up_tenth) +
        ((int32_t)g_config.load_height_mm * 10);
    sensor_height_tenth = (int32_t)(
        (g_bunker_config.lower_height_mm +
         g_bunker_config.cylinder_height_mm +
         g_bunker_config.sensor_height_above_base_mm) * 10UL);
    g_result_tenth_mm =
        (sensor_height_tenth > measured_distance_tenth) ?
        (sensor_height_tenth - measured_distance_tenth) : 0;
    g_status.travel_difference = difference;

    return encoder_motor_difference_tenth;
}

static void live_counters_zero(uint32_t now)
{
    const uint32_t primask = __get_PRIMASK();

    __disable_irq();
    __HAL_TIM_SET_COUNTER(&g_tim3_encoder, 0U);
    g_encoder_previous = 0U;
    g_fg_count = 0;
    g_fg_step_sign = 1;
    g_fg_last_ms = now;
    if (primask == 0U)
    {
        __enable_irq();
    }

    g_status.encoder_position = 0;
    g_status.fg_count = 0;
    g_last_encoder_ms = now;
}

static void live_view_clear(void)
{
    g_live_view_active = false;
    g_indicator_depth = 0;
    g_indicator_climb = 0;
}

static void measurement_counters_reset(uint32_t now, bool clear_result)
{
    live_counters_zero(now);

    g_status.descent_distance = 0;
    g_status.descent_fg_count = 0;
    g_status.last_distance = 0;
    g_status.ascent_distance = 0;
    g_status.travel_difference = 0;
    g_down_sign = 0;
    g_measure_origin = 0;
    g_live_down_counts = 0;
    g_live_up_counts = 0;
    g_indicator_depth = 0;
    g_indicator_climb = 0;
    g_live_view_active = false;
    g_saved_down_counts = 0;
    g_saved_up_counts = 0;
    g_saved_down_fg_counts = 0;
    g_saved_up_fg_counts = 0;
    g_contact_fg_count = 0;
    g_calibration_capture_flags = 0U;
    g_saved_descent_tenth_mm = 0;
    g_saved_ascent_tenth_mm = 0;
    g_path_difference_tenth_mm = 0;
    if (clear_result)
    {
        g_result_tenth_mm = 0;
    }
    g_status.errors &= (uint16_t)~LOT_ERROR_MOTOR_LENGTH_TEST;
    g_status.encoder_speed_cps = 0U;
    g_speed_window_counts = 0U;
    g_speed_window_started_ms = now;
}

static void motor_stop(void)
{
    __HAL_TIM_SET_COMPARE(&g_tim1_pwm, TIM_CHANNEL_1, 0U);
    g_status.motor_enabled = false;
    g_status.pwm = 0U;
}

static void motor_start(bool up, uint16_t pwm)
{
    if (pwm > 1000U)
    {
        pwm = 1000U;
    }
    HAL_GPIO_WritePin(MOT_DIR_WHITE_PORT, MOT_DIR_WHITE_PIN,
                      up ? GPIO_PIN_SET : GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&g_tim1_pwm, TIM_CHANNEL_1, pwm);
    g_status.motor_up = up;
    g_status.motor_enabled = (pwm != 0U);
    g_status.pwm = pwm;
    g_fg_step_sign = up ? 1 : -1;
}

static void diagnostics_state_enter(lot_state_t state, uint32_t now)
{
    if ((state == LOT_STATE_PARKING) && g_measurement_requested)
    {
        g_diagnostics_cycle_active = true;
        g_diagnostics_descent_started = false;
        g_diagnostics_ascent_started = false;
        g_diagnostics_cycle_started_ms = now;
    }
    else if ((state == LOT_STATE_DESCENDING) &&
             g_diagnostics_cycle_active &&
             !g_diagnostics_descent_started)
    {
        g_diagnostics_parking_ms = now - g_diagnostics_cycle_started_ms;
        g_diagnostics_descent_started_ms = now;
        g_diagnostics_descent_started = true;
    }
    else if ((state == LOT_STATE_SLACK_TAKEUP) &&
             g_diagnostics_cycle_active &&
             g_diagnostics_descent_started &&
             !g_diagnostics_ascent_started)
    {
        g_diagnostics_descent_ms =
            now - g_diagnostics_descent_started_ms;
        g_diagnostics_ascent_started_ms = now;
        g_diagnostics_ascent_started = true;
    }
}

static void diagnostics_measurement_complete(uint32_t now)
{
    if (!g_diagnostics_cycle_active ||
        !g_diagnostics_descent_started ||
        !g_diagnostics_ascent_started)
    {
        g_diagnostics_cycle_active = false;
        return;
    }

    g_diagnostics_ascent_ms = now - g_diagnostics_ascent_started_ms;
    g_diagnostics_measurement_ms =
        now - g_diagnostics_descent_started_ms;
    g_diagnostics_cycle_ms = now - g_diagnostics_cycle_started_ms;
    g_diagnostics_descent_retries = g_descent_start_retries;
    g_diagnostics_park_retries = g_park_test_start_retries;
    ++g_diagnostics_sequence;
    g_diagnostics_cycle_active = false;
}

static void enter_state(lot_state_t state, uint32_t now)
{
    diagnostics_state_enter(state, now);
    g_status.state = state;
    g_state_started_ms = now;
    g_state_encoder_start = g_status.encoder_position;
    g_state_fg_start = g_fg_count;
    g_state_encoder_seen = false;
    g_state_up_seen = false;

    switch (state)
    {
        case LOT_STATE_MANUAL_UP:
            motor_start(true, g_manual_pwm);
            break;
        case LOT_STATE_MANUAL_DOWN:
            if (ground_sensor_raw_active())
            {
                motor_stop();
            }
            else
            {
                motor_start(false, g_manual_pwm);
            }
            break;
        case LOT_STATE_PARKING:
            motor_start(true, g_config.park_pwm);
            break;
        case LOT_STATE_DESCENDING:
            g_live_view_active = true;
            g_measure_origin = g_status.encoder_position;
            g_indicator_depth = 0;
            g_indicator_climb = 0;
            g_status.descent_distance = 0;
            g_status.encoder_speed_cps = 0U;
            g_speed_window_counts = 0U;
            g_speed_window_started_ms = now;
            ground_sensor_reset();
            motor_start(false, g_config.run_pwm);
            break;
        case LOT_STATE_CONTACT:
            motor_stop();
            break;
        case LOT_STATE_SLACK_TAKEUP:
            g_live_up_counts = 0;
            motor_start(true, g_config.takeup_pwm);
            break;
        case LOT_STATE_ASCENT_RAMP:
            motor_start(true, g_config.takeup_pwm);
            break;
        case LOT_STATE_ASCENDING:
            motor_start(true, g_config.run_pwm);
            break;
        case LOT_STATE_PARK_RELEASE:
            g_park_test_start_retries = 0U;
            g_park_test_retry_waiting = false;
            motor_stop();
            break;
        case LOT_STATE_PARK_TEST_STOP:
        case LOT_STATE_MEASURE_START_WAIT:
        case LOT_STATE_INTERLOCK_WAIT:
        case LOT_STATE_INTERLOCK_STOP:
            motor_stop();
            break;
        case LOT_STATE_PARK_TEST_DOWN:
            motor_start(false, g_config.park_pwm);
            break;
        case LOT_STATE_PARK_RETURN:
            g_descent_start_retries = 0U;
            motor_start(true, g_config.park_pwm);
            break;
        default:
            motor_stop();
            break;
    }
}

static void set_fault(uint16_t error, uint32_t now)
{
    live_view_clear();
    g_diagnostics_cycle_active = false;
    g_status.errors |= error;
    g_measurement_requested = false;
    g_skip_post_measure_park = false;
    enter_state(LOT_STATE_FAULT, now);
}

static bool start_command(lot_command_t command, uint32_t now)
{
    if (command == LOT_CMD_STOP)
    {
        g_status.auto_enabled = false;
        g_local_auto_enabled = false;
        g_measurement_requested = false;
        g_skip_post_measure_park = false;
        g_encoder_mismatch_count = 0U;
        g_diagnostics_cycle_active = false;
        if (g_status.errors != LOT_ERROR_NONE)
        {
            enter_state(LOT_STATE_FAULT, now);
        }
        else if (interlock_input_active())
        {
            enter_state(LOT_STATE_INTERLOCK_WAIT, now);
        }
        else
        {
            enter_state(LOT_STATE_READY, now);
        }
        return true;
    }

    if (g_status.state == LOT_STATE_FAULT)
    {
        return false;
    }
    if (interlock_input_active() && (command != LOT_CMD_PARK))
    {
        return false;
    }

    switch (command)
    {
        case LOT_CMD_MOVE_UP:
            enter_state(LOT_STATE_MANUAL_UP, now);
            return true;
        case LOT_CMD_MOVE_DOWN:
            enter_state(LOT_STATE_MANUAL_DOWN, now);
            return true;
        case LOT_CMD_PARK:
            g_measurement_requested = false;
            g_skip_post_measure_park = false;
            enter_state(LOT_STATE_PARKING, now);
            return true;
        case LOT_CMD_MEASURE_AUTO_ONCE:
            if (g_status.state == LOT_STATE_READY)
            {
                g_measurement_requested = true;
                g_skip_post_measure_park = true;
                enter_state(LOT_STATE_PARKING, now);
                return true;
            }
            return false;
        default:
            return false;
    }
}

static uint8_t buttons_read(void)
{
    uint8_t mask = 0U;
    uint8_t index;

    for (index = 0U; index < BUTTON_COUNT; ++index)
    {
        if (HAL_GPIO_ReadPin(GPIOA, g_button_pins[index]) == GPIO_PIN_RESET)
        {
            mask |= (uint8_t)(1U << index);
        }
    }
    return mask;
}

static void state_led_update(uint32_t now)
{
    bool led_on;

    if ((g_status.state == LOT_STATE_FAULT) ||
        (g_status.errors != LOT_ERROR_NONE))
    {
        led_on = ((now / STATE_LED_FAULT_HALF_MS) & 1U) == 0U;
    }
    else if (g_status.state == LOT_STATE_READY)
    {
        led_on = true;
    }
    else if (g_status.state == LOT_STATE_INTERLOCK_WAIT)
    {
        led_on = false;
    }
    else
    {
        led_on = ((now / STATE_LED_BUSY_HALF_MS) & 1U) == 0U;
    }

    HAL_GPIO_WritePin(STATE_LED_PORT, STATE_LED_PIN,
                      led_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static bool interlock_input_active(void)
{
    return HAL_GPIO_ReadPin(INTERLOCK_INPUT_PORT,
                            INTERLOCK_INPUT_PIN) == GPIO_PIN_RESET;
}

static bool sensor_busy(void)
{
    return (g_status.state != LOT_STATE_READY) &&
           (g_status.state != LOT_STATE_INTERLOCK_WAIT);
}

static void busy_output_update(void)
{
    HAL_GPIO_WritePin(BUSY_OUTPUT_PORT, BUSY_OUTPUT_PIN,
                      sensor_busy() ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void interlock_tick(uint32_t now)
{
    if (!interlock_input_active())
    {
        if (g_status.state == LOT_STATE_INTERLOCK_WAIT)
        {
            enter_state(LOT_STATE_READY, now);
        }
        return;
    }

    g_measurement_requested = false;
    g_skip_post_measure_park = false;

    switch (g_status.state)
    {
        case LOT_STATE_INIT:
        case LOT_STATE_READY:
            enter_state(LOT_STATE_INTERLOCK_WAIT, now);
            break;

        case LOT_STATE_FAULT:
        case LOT_STATE_INTERLOCK_WAIT:
        case LOT_STATE_INTERLOCK_STOP:
        case LOT_STATE_PARKING:
        case LOT_STATE_PARK_RETURN:
            break;

        default:
            live_view_clear();
            enter_state(LOT_STATE_INTERLOCK_STOP, now);
            break;
    }
}

static void buttons_tick(uint32_t now)
{
    const uint8_t raw = buttons_read();
    uint8_t index;

    for (index = 0U; index < BUTTON_COUNT; ++index)
    {
        const uint8_t bit = (uint8_t)(1U << index);
        const bool raw_pressed = (raw & bit) != 0U;
        const bool candidate_pressed = (g_button_candidate_mask & bit) != 0U;

        if (raw_pressed != candidate_pressed)
        {
            g_button_candidate_mask ^= bit;
            g_button_debounce[index] = 0U;
        }
        else if (g_button_debounce[index] < BUTTON_DEBOUNCE_TICKS)
        {
            ++g_button_debounce[index];
            if (g_button_debounce[index] == BUTTON_DEBOUNCE_TICKS)
            {
                const bool old_pressed = (g_button_stable_mask & bit) != 0U;
                if (raw_pressed)
                {
                    g_button_stable_mask |= bit;
                }
                else
                {
                    g_button_stable_mask &= (uint8_t)~bit;
                }

                if (raw_pressed && !old_pressed)
                {
                    if (index == 0U)
                    {
                        (void)start_command(LOT_CMD_MOVE_UP, now);
                    }
                    else if (index == 1U)
                    {
                        (void)start_command(LOT_CMD_MOVE_DOWN, now);
                    }
                    else if (index == 2U)
                    {
                        (void)start_command(LOT_CMD_PARK, now);
                    }
                    else if (index == 3U)
                    {
                        (void)start_command(LOT_CMD_MEASURE_AUTO_ONCE, now);
                    }
                    else if (index == 4U)
                    {
                        g_status.auto_enabled = !g_status.auto_enabled;
                        g_local_auto_enabled = g_status.auto_enabled;
                        g_next_auto_ms = now;
                    }
                }
                else if (!raw_pressed && old_pressed)
                {
                    if (((index == 0U) &&
                         (g_status.state == LOT_STATE_MANUAL_UP)) ||
                        ((index == 1U) &&
                         (g_status.state == LOT_STATE_MANUAL_DOWN)))
                    {
                        (void)start_command(LOT_CMD_STOP, now);
                    }
                }
            }
        }
    }
}

static int32_t encoder_counter_delta(uint16_t current, uint16_t previous)
{
    int32_t delta = (int32_t)current - (int32_t)previous;

    if (delta > 32768)
    {
        delta -= 65536;
    }
    else if (delta < -32768)
    {
        delta += 65536;
    }

    return delta;
}

static bool encoder_idle_for(uint32_t now, uint32_t elapsed, uint32_t idle_ms)
{
    if (idle_ms == 0U)
    {
        return false;
    }

    return (elapsed > idle_ms) &&
           ((now - g_last_encoder_ms) >= idle_ms);
}

static bool motion_stalled_for(uint32_t now, uint32_t elapsed,
                               uint32_t idle_ms)
{
    if (g_config.motor_encoder_enabled == 0U)
    {
        return encoder_idle_for(now, elapsed, idle_ms);
    }
    if (idle_ms == 0U)
    {
        return false;
    }

    return (elapsed > idle_ms) &&
           ((now - g_fg_last_ms) >= idle_ms) &&
           ((now - g_last_encoder_ms) >= idle_ms);
}

static bool upper_stop_reached(uint32_t now, uint32_t elapsed)
{
    if (g_config.motor_encoder_enabled == 0U)
    {
        return encoder_idle_for(now, elapsed, g_config.stall_timeout_ms);
    }

    return (elapsed > g_config.stall_timeout_ms) &&
           ((now - g_fg_last_ms) >= g_config.stall_timeout_ms);
}

static void ground_sensor_reset(void)
{
    g_ground_sensor_active = false;
}

static void ground_sensor_tick(uint32_t now)
{
    (void)now;

    g_ground_sensor_active = ground_sensor_raw_active();

    if (((g_status.state == LOT_STATE_DESCENDING) ||
         (g_status.state == LOT_STATE_MANUAL_DOWN)) &&
        g_ground_sensor_active)
    {
        motor_stop();
    }
}

static bool ground_sensor_descent_hit(uint32_t elapsed)
{
    (void)elapsed;
    return ground_sensor_raw_active();
}

static void enter_descent_contact(uint32_t now, bool gpio_instant_takeup)
{
    int32_t fg_travelled = g_fg_count - g_state_fg_start;

    if (fg_travelled < 0)
    {
        fg_travelled = -fg_travelled;
    }
    g_saved_down_counts = g_live_down_counts;
    g_saved_down_fg_counts = fg_travelled;
    g_contact_fg_count = g_fg_count;
    g_calibration_capture_flags |= 0x01U;
    g_status.last_distance = g_saved_down_counts;
    g_status.descent_fg_count = fg_travelled;
    g_down_sign =
        ((g_status.encoder_position - g_state_encoder_start) < 0) ? -1 : 1;

    /* Индуктивный датчик: сразу подъём на takeup_pwm без паузы 150 мс.
     * По скорости энкодера — пауза CONTACT_REVERSAL_PAUSE_MS для стабилизации. */
    if (gpio_instant_takeup)
    {
        enter_state(LOT_STATE_SLACK_TAKEUP, now);
    }
    else
    {
        enter_state(LOT_STATE_CONTACT, now);
    }
}

static void encoder_tick(uint32_t now)
{
    const uint16_t current =
        (uint16_t)__HAL_TIM_GET_COUNTER(&g_tim3_encoder);
    const int32_t delta =
        encoder_counter_delta(current, g_encoder_previous);
    const uint32_t speed_elapsed = now - g_speed_window_started_ms;

    g_encoder_previous = current;
    if (delta != 0)
    {
        uint32_t absolute_delta = (uint32_t)delta;
        if (delta < 0)
        {
            absolute_delta = (uint32_t)(-delta);
        }
        g_status.encoder_position += delta;
        g_speed_window_counts += absolute_delta;
        g_last_encoder_ms = now;
        g_state_encoder_seen = true;
        if ((g_status.state == LOT_STATE_CONTACT) ||
                 (g_status.state == LOT_STATE_SLACK_TAKEUP) ||
                 (g_status.state == LOT_STATE_ASCENT_RAMP) ||
                 (g_status.state == LOT_STATE_ASCENDING))
        {
            if (((delta > 0) && (g_down_sign > 0)) ||
                ((delta < 0) && (g_down_sign < 0)))
            {
                if (g_status.state == LOT_STATE_SLACK_TAKEUP)
                {
                    if (g_live_up_counts > (int32_t)absolute_delta)
                    {
                        g_live_up_counts -= (int32_t)absolute_delta;
                    }
                    else
                    {
                        g_live_up_counts = 0;
                    }
                }
            }
            else if (g_status.state != LOT_STATE_CONTACT)
            {
                g_live_up_counts += (int32_t)absolute_delta;
                g_state_up_seen = true;
            }
        }
    }
    if (speed_elapsed >= SPEED_SAMPLE_MS)
    {
        g_status.encoder_speed_cps =
            (g_speed_window_counts * 1000UL) / speed_elapsed;
        g_speed_window_counts = 0U;
        g_speed_window_started_ms = now;
    }
    if ((g_config.motor_encoder_enabled != 0U) &&
        ((g_status.state == LOT_STATE_SLACK_TAKEUP) ||
         (g_status.state == LOT_STATE_ASCENT_RAMP) ||
         (g_status.state == LOT_STATE_ASCENDING)) &&
        (g_fg_count != g_contact_fg_count))
    {
        g_state_up_seen = true;
    }
    g_status.fg_count = g_fg_count;
}

static void motion_tick(uint32_t now)
{
    const uint32_t elapsed = now - g_state_started_ms;
    const uint32_t encoder_idle = now - g_last_encoder_ms;
    const uint32_t fg_idle = now - g_fg_last_ms;
    int32_t travelled = g_status.encoder_position - g_state_encoder_start;

    if (g_live_view_active &&
        ((g_status.state == LOT_STATE_DESCENDING) ||
         (g_status.state == LOT_STATE_CONTACT) ||
         (g_status.state == LOT_STATE_SLACK_TAKEUP) ||
         (g_status.state == LOT_STATE_ASCENT_RAMP) ||
         (g_status.state == LOT_STATE_ASCENDING)))
    {
        int32_t depth = g_status.encoder_position - g_measure_origin;

        if (depth < 0)
        {
            depth = -depth;
        }
        g_indicator_depth = depth;
        g_indicator_climb = g_live_up_counts;
    }

    if (travelled < 0)
    {
        travelled = -travelled;
    }
    if (g_status.state == LOT_STATE_DESCENDING)
    {
        int32_t depth = g_status.encoder_position - g_measure_origin;

        if (depth < 0)
        {
            depth = -depth;
        }
        g_live_down_counts = depth;
    }

    switch (g_status.state)
    {
        case LOT_STATE_INIT:
            enter_state(LOT_STATE_READY, now);
            break;

        case LOT_STATE_MANUAL_UP:
            break;

        case LOT_STATE_MANUAL_DOWN:
            if (ground_sensor_raw_active())
            {
                motor_stop();
            }
            break;

        case LOT_STATE_PARKING:
            if (elapsed >= g_config.motion_timeout_ms)
            {
                set_fault(LOT_ERROR_PARK_NOT_FOUND, now);
            }
            else if (upper_stop_reached(now, elapsed))
            {
                if (g_measurement_requested && !g_status.auto_enabled)
                {
                    motor_stop();
                    live_counters_zero(now);
                    g_descent_start_retries = 0U;
                    enter_state(LOT_STATE_MEASURE_START_WAIT, now);
                }
                else if (interlock_input_active() &&
                         !g_measurement_requested)
                {
                    motor_stop();
                    live_counters_zero(now);
                    enter_state(LOT_STATE_INTERLOCK_WAIT, now);
                }
                else
                {
                    enter_state(LOT_STATE_PARK_RELEASE, now);
                }
            }
            break;

        case LOT_STATE_DESCENDING:
        {
            bool gpio_contact = false;
            int32_t fg_descent = g_fg_count - g_state_fg_start;
            int32_t selected_descent_tenth;

            if (fg_descent < 0)
            {
                fg_descent = -fg_descent;
            }
            selected_descent_tenth =
                (g_config.motor_encoder_enabled != 0U) ?
                motor_pulses_to_tenth_mm(fg_descent) :
                encoder_pulses_to_tenth_mm(g_live_down_counts);

            if (selected_descent_tenth >=
                (int32_t)(g_config.max_descent_mm * 10UL))
            {
                set_fault(LOT_ERROR_DESCENT_LIMIT, now);
                break;
            }
            if (elapsed >= g_config.motion_timeout_ms)
            {
                set_fault(LOT_ERROR_MOTION_TIMEOUT, now);
                break;
            }
            g_status.descent_distance = g_live_down_counts;
            gpio_contact = ground_sensor_descent_hit(elapsed);
            if ((g_config.motor_encoder_enabled != 0U) &&
                (elapsed > g_config.stall_timeout_ms) &&
                     (fg_idle >= g_config.stall_timeout_ms))
            {
                set_fault(LOT_ERROR_MOT_FG_YELLOW_STALLED, now);
            }
            else if ((g_config.motor_encoder_enabled == 0U) &&
                     !g_state_encoder_seen &&
                     (elapsed > DESCENT_START_RETRY_MS))
            {
                if (g_descent_start_retries < DESCENT_START_MAX_RETRIES)
                {
                    ++g_descent_start_retries;
                    motor_stop();
                    enter_state(LOT_STATE_MEASURE_START_WAIT, now);
                }
                else
                {
                    set_fault(LOT_ERROR_ENCODER_MISSING, now);
                }
            }
            else
            {
                if (gpio_contact)
                {
                    enter_descent_contact(now, true);
                }
            }
            break;
        }

        case LOT_STATE_CONTACT:
            if (elapsed >= CONTACT_REVERSAL_PAUSE_MS)
            {
                enter_state(LOT_STATE_SLACK_TAKEUP, now);
            }
            break;

        case LOT_STATE_SLACK_TAKEUP:
        {
            int32_t fg_up = g_fg_count - g_contact_fg_count;
            int32_t selected_up_tenth;

            if (fg_up < 0)
            {
                fg_up = -fg_up;
            }
            selected_up_tenth =
                (g_config.motor_encoder_enabled != 0U) ?
                motor_pulses_to_tenth_mm(fg_up) :
                encoder_pulses_to_tenth_mm(g_live_up_counts);
            if (selected_up_tenth >= TAKEUP_CONFIRM_TENTH_MM)
            {
                enter_state(LOT_STATE_ASCENT_RAMP, now);
            }
            else if (elapsed >= TAKEUP_TIMEOUT_MS)
            {
                set_fault(LOT_ERROR_ASCENT_NOT_STARTED, now);
            }
            break;
        }

        case LOT_STATE_ASCENT_RAMP:
        {
            int32_t fg_up = g_fg_count - g_contact_fg_count;
            int32_t up_tenth;
            int32_t down_tenth;
            int32_t stop_enable_tenth;

            if (fg_up < 0)
            {
                fg_up = -fg_up;
            }
            up_tenth = (g_config.motor_encoder_enabled != 0U) ?
                motor_pulses_to_tenth_mm(fg_up) :
                encoder_pulses_to_tenth_mm(g_live_up_counts);
            down_tenth = (g_config.motor_encoder_enabled != 0U) ?
                motor_pulses_to_tenth_mm(g_saved_down_fg_counts) :
                encoder_pulses_to_tenth_mm(g_saved_down_counts);
            stop_enable_tenth = down_tenth / 5;

            if (stop_enable_tenth > ASCENT_RAMP_TENTH_MM)
            {
                stop_enable_tenth = ASCENT_RAMP_TENTH_MM;
            }
            if (stop_enable_tenth < TAKEUP_CONFIRM_TENTH_MM)
            {
                stop_enable_tenth = TAKEUP_CONFIRM_TENTH_MM;
            }
            if (up_tenth >= stop_enable_tenth)
            {
                enter_state(LOT_STATE_ASCENDING, now);
                g_state_up_seen = true;
            }
            break;
        }

        case LOT_STATE_ASCENDING:
        {
            int32_t fg_up = g_fg_count - g_contact_fg_count;
            int32_t selected_up_tenth;
            int32_t selected_down_tenth;

            if (fg_up < 0)
            {
                fg_up = -fg_up;
            }
            selected_up_tenth =
                (g_config.motor_encoder_enabled != 0U) ?
                motor_pulses_to_tenth_mm(fg_up) :
                encoder_pulses_to_tenth_mm(g_live_up_counts);
            selected_down_tenth =
                (g_config.motor_encoder_enabled != 0U) ?
                motor_pulses_to_tenth_mm(g_saved_down_fg_counts) :
                encoder_pulses_to_tenth_mm(g_live_down_counts);
            if (elapsed >= g_config.motion_timeout_ms)
            {
                (void)finish_encoder_lengths();
                set_fault(LOT_ERROR_MOTION_TIMEOUT, now);
                break;
            }
            g_status.ascent_distance = g_live_up_counts;
            if ((selected_down_tenth > 0) &&
                (selected_up_tenth >=
                 (selected_down_tenth *
                  (int32_t)(100U - g_config.slowdown_remaining_percent)) /
                 100))
            {
                const uint16_t slow_pwm = (uint16_t)(
                    ((uint32_t)g_config.run_pwm * 10UL) /
                    g_config.slowdown_divisor_tenths);
                const uint16_t limited_pwm =
                    (slow_pwm < MOTOR_MIN_ACTIVE_PWM) ?
                    MOTOR_MIN_ACTIVE_PWM : slow_pwm;
                if (g_status.pwm != limited_pwm)
                {
                    motor_start(true, limited_pwm);
                }
            }

            if (g_state_up_seen &&
                     (encoder_idle >= g_config.contact_timeout_ms) &&
                     ((g_config.motor_encoder_enabled == 0U) ||
                      (fg_idle >= g_config.stall_timeout_ms)))
            {
                const int32_t encoder_motor_difference_tenth =
                    finish_encoder_lengths();
                bool encoder_mismatch_fault = false;

                diagnostics_measurement_complete(now);
                live_view_clear();
                ++g_status.measurement_count;
                if (g_config.mismatch_mm == 0U)
                {
                    g_encoder_mismatch_count = 0U;
                }
                else if (encoder_motor_difference_tenth >
                         ((int32_t)g_config.mismatch_mm * 10))
                {
                    if (g_encoder_mismatch_count < UINT16_MAX)
                    {
                        ++g_encoder_mismatch_count;
                    }
                    encoder_mismatch_fault =
                        g_encoder_mismatch_count >=
                        g_config.error_measurement_limit;
                }
                else
                {
                    g_encoder_mismatch_count = 0U;
                }
                live_counters_zero(now);
                g_measurement_requested = false;
                g_next_auto_ms = now +
                    (g_local_auto_enabled ?
                     LOCAL_AUTO_INTERVAL_MS : g_config.auto_interval_ms);
                if (encoder_mismatch_fault)
                {
                    g_status.auto_enabled = false;
                    g_local_auto_enabled = false;
                    g_skip_post_measure_park = false;
                    set_fault(LOT_ERROR_TRAVEL_MISMATCH, now);
                    break;
                }
                if (g_skip_post_measure_park)
                {
                    g_skip_post_measure_park = false;
                    enter_state(LOT_STATE_READY, now);
                }
                else
                {
                    enter_state(LOT_STATE_PARK_RELEASE, now);
                }
            }
            break;
        }

        case LOT_STATE_PARK_RELEASE:
        {
            const uint32_t release_wait_ms =
                (g_config.park_release_ms < PARK_RELEASE_MIN_MS) ?
                PARK_RELEASE_MIN_MS : (uint32_t)g_config.park_release_ms;

            if (elapsed >= release_wait_ms)
            {
                enter_state(LOT_STATE_PARK_TEST_DOWN, now);
            }
            break;
        }

        case LOT_STATE_PARK_TEST_DOWN:
        {
            const uint32_t park_stall_idle =
                (uint32_t)g_config.stall_timeout_ms +
                (uint32_t)g_config.contact_timeout_ms;
            int32_t fg_delta = g_fg_count - g_state_fg_start;
            int32_t fg_abs;
            bool park_test_limit_reached;

            if (g_park_test_retry_waiting)
            {
                if ((now - g_park_test_retry_started_ms) >=
                    PARK_TEST_START_RETRY_MS)
                {
                    g_state_started_ms = now;
                    g_state_encoder_start = g_status.encoder_position;
                    g_state_fg_start = g_fg_count;
                    g_state_encoder_seen = false;
                    g_park_test_retry_waiting = false;
                    motor_start(false, g_config.park_pwm);
                }
                break;
            }

            if (fg_delta < 0)
            {
                fg_delta = -fg_delta;
            }
            fg_abs = fg_delta;
            park_test_limit_reached =
                (g_config.motor_encoder_enabled != 0U) ?
                (motor_pulses_to_tenth_mm(fg_abs) >=
                 encoder_pulses_to_tenth_mm(g_config.park_test_pulses)) :
                ((uint32_t)travelled >= g_config.park_test_pulses);

            if (park_test_limit_reached)
            {
                enter_state(LOT_STATE_PARK_TEST_STOP, now);
            }
            else if (elapsed >= g_config.motion_timeout_ms)
            {
                set_fault(LOT_ERROR_MOTION_TIMEOUT, now);
            }
            else if (motion_stalled_for(now, elapsed, park_stall_idle) &&
                     g_state_encoder_seen)
            {
                enter_state(LOT_STATE_PARK_TEST_STOP, now);
            }
            else if (!g_state_encoder_seen &&
                     (fg_abs <= (int32_t)PARK_FG_MOVEMENT_PULSES) &&
                     (elapsed >= PARK_TEST_START_RETRY_MS))
            {
                if (g_park_test_start_retries < PARK_TEST_MAX_RETRIES)
                {
                    ++g_park_test_start_retries;
                    motor_stop();
                    g_park_test_retry_waiting = true;
                    g_park_test_retry_started_ms = now;
                }
                else
                {
                    set_fault(LOT_ERROR_PARK_TEST_FAILED, now);
                }
            }
            else if (elapsed >= g_config.park_test_ms)
            {
                if ((g_config.motor_encoder_enabled != 0U) &&
                    (fg_abs > 0))
                {
                    enter_state(LOT_STATE_PARK_TEST_STOP, now);
                }
                else if (g_state_encoder_seen && ((uint32_t)travelled > 0U))
                {
                    enter_state(LOT_STATE_PARK_TEST_STOP, now);
                }
                else if ((g_config.motor_encoder_enabled == 0U) &&
                         (fg_abs > (int32_t)PARK_FG_MOVEMENT_PULSES) &&
                         !g_state_encoder_seen)
                {
                    set_fault(LOT_ERROR_LOAD_JAMMED, now);
                }
                else
                {
                    set_fault(LOT_ERROR_PARK_TEST_FAILED, now);
                }
            }
            break;
        }

        case LOT_STATE_PARK_TEST_STOP:
            if (elapsed >= 150U)
            {
                enter_state(LOT_STATE_PARK_RETURN, now);
            }
            break;

        case LOT_STATE_PARK_RETURN:
            if (elapsed >= g_config.motion_timeout_ms)
            {
                set_fault(LOT_ERROR_PARK_NOT_FOUND, now);
            }
            else if (upper_stop_reached(now, elapsed))
            {
                motor_stop();
                live_counters_zero(now);
                if (interlock_input_active())
                {
                    g_measurement_requested = false;
                    enter_state(LOT_STATE_INTERLOCK_WAIT, now);
                }
                else if (g_measurement_requested)
                {
                    enter_state(LOT_STATE_MEASURE_START_WAIT, now);
                }
                else
                {
                    enter_state(LOT_STATE_READY, now);
                }
            }
            break;

        case LOT_STATE_MEASURE_START_WAIT:
        {
            const uint32_t release_wait_ms =
                (g_config.park_release_ms < PARK_RELEASE_MIN_MS) ?
                PARK_RELEASE_MIN_MS : (uint32_t)g_config.park_release_ms;

            if (elapsed >= release_wait_ms)
            {
                measurement_counters_reset(now, false);
                enter_state(LOT_STATE_DESCENDING, now);
            }
            break;
        }

        case LOT_STATE_INTERLOCK_STOP:
            if (elapsed >= MEASURE_REVERSAL_PAUSE_MS)
            {
                enter_state(LOT_STATE_PARKING, now);
            }
            break;

        case LOT_STATE_INTERLOCK_WAIT:
            motor_stop();
            break;

        case LOT_STATE_READY:
            if (g_status.auto_enabled &&
                ((int32_t)(now - g_next_auto_ms) >= 0))
            {
                g_measurement_requested = true;
                g_skip_post_measure_park = true;
                enter_state(LOT_STATE_PARKING, now);
            }
            break;

        case LOT_STATE_FAULT:
        default:
            motor_stop();
            break;
    }
}

static void app_tick(uint32_t now)
{
    encoder_tick(now);
    interlock_tick(now);
    buttons_tick(now);
    ground_sensor_tick(now);
    motion_tick(now);
}

static void status_payload(uint8_t *payload, uint8_t *length)
{
    uint8_t flags = 0U;

    if (g_status.auto_enabled)
    {
        flags |= 0x01U;
    }
    if (g_status.motor_enabled)
    {
        flags |= 0x02U;
    }
    if (g_status.motor_up)
    {
        flags |= 0x04U;
    }
    if (g_ground_sensor_active)
    {
        flags |= 0x08U;
    }
    if (interlock_input_active())
    {
        flags |= 0x10U;
    }
    if (sensor_busy())
    {
        flags |= 0x20U;
    }

    uint16_t reported_errors = g_status.errors;

    if (g_uart_glitch_pending != 0U)
    {
        reported_errors |= LOT_ERROR_UART;
        g_uart_glitch_pending = 0U;
    }

    payload[0] = LOT_RESULT_OK;
    payload[1] = (uint8_t)g_status.state;
    write_u16(&payload[2], reported_errors);
    payload[4] = flags;
    write_u32(&payload[5], (uint32_t)g_status.encoder_position);
    write_u32(&payload[9], (uint32_t)g_status.fg_count);
    write_u16(&payload[13], g_status.pwm);
    write_u32(&payload[15], (uint32_t)g_status.last_distance);
    write_u32(&payload[19], g_status.measurement_count);
    payload[23] = g_button_stable_mask;
    write_u32(&payload[24], (uint32_t)encoder_counts_to_tenth_mm(
        g_status.encoder_position));
    write_u32(&payload[28], (uint32_t)motor_pulses_to_tenth_mm(
        g_status.fg_count));
    write_u32(&payload[32], g_status.encoder_speed_cps);
    write_u32(&payload[36], (uint32_t)encoder_pulses_to_tenth_mm(
        g_indicator_depth));
    write_u32(&payload[40], (uint32_t)encoder_pulses_to_tenth_mm(
        g_indicator_climb));
    write_u32(&payload[44], (uint32_t)g_path_difference_tenth_mm);
    write_u32(&payload[48], (uint32_t)motor_pulses_to_tenth_mm(
        g_status.descent_fg_count));
    write_u32(&payload[52], (uint32_t)g_saved_descent_tenth_mm);
    write_u32(&payload[56], (uint32_t)g_result_tenth_mm);
    write_u32(&payload[60], (uint32_t)g_saved_ascent_tenth_mm);
    *length = 64U;
}

static void calibration_capture_payload(uint8_t *payload, uint8_t *length)
{
    payload[0] = LOT_RESULT_OK;
    payload[1] = g_calibration_capture_flags;
    write_u32(&payload[2], (uint32_t)g_saved_down_counts);
    write_u32(&payload[6], (uint32_t)g_saved_up_counts);
    write_u32(&payload[10], (uint32_t)g_saved_down_fg_counts);
    write_u32(&payload[14], (uint32_t)g_saved_up_fg_counts);
    *length = 18U;
}

static void diagnostics_payload(uint8_t *payload, uint8_t *length)
{
    payload[0] = LOT_RESULT_OK;
    write_u32(&payload[1], g_diagnostics_sequence);
    write_u32(&payload[5], g_diagnostics_cycle_ms);
    write_u32(&payload[9], g_diagnostics_measurement_ms);
    write_u32(&payload[13], g_diagnostics_parking_ms);
    write_u32(&payload[17], g_diagnostics_descent_ms);
    write_u32(&payload[21], g_diagnostics_ascent_ms);
    write_u16(&payload[25], g_diagnostics_descent_retries);
    write_u16(&payload[27], g_diagnostics_park_retries);
    *length = 29U;
}

static void config_payload(uint8_t *payload, uint8_t *length)
{
    payload[0] = LOT_RESULT_OK;
    write_u16(&payload[1], g_config.run_pwm);
    write_u16(&payload[3], g_config.park_pwm);
    write_u16(&payload[5], g_config.contact_timeout_ms);
    write_u16(&payload[7], g_config.stall_timeout_ms);
    write_u16(&payload[9], g_config.mismatch_mm);
    write_u16(&payload[11], g_config.park_release_ms);
    write_u16(&payload[13], g_config.park_test_ms);
    write_u16(&payload[15], g_config.park_test_pulses);
    write_u16(&payload[17], g_config.contact_speed_percent);
    write_u16(&payload[19], g_config.contact_ignore_ms);
    write_u16(&payload[21], g_config.slowdown_remaining_percent);
    write_u16(&payload[23], g_config.slowdown_divisor_tenths);
    write_u32(&payload[25], g_config.auto_interval_ms);
    write_u32(&payload[29], g_config.length_cal_x10000);
    write_u32(&payload[33], g_config.motor_cal_x10000);
    write_u32(&payload[37], (uint32_t)g_config.length_cal_offset_tenth_mm);
    write_u16(&payload[41], g_config.brake_comp_mm);
    write_u16(&payload[43], g_config.load_height_mm);
    write_u16(&payload[45], g_config.motor_encoder_enabled);
    write_u16(&payload[47], (uint16_t)g_config.descent_correction_mm);
    write_u16(&payload[49], g_config.error_measurement_limit);
    write_u32(&payload[51], g_config.motion_timeout_ms);
    write_u32(&payload[55], g_config.max_descent_mm);
    write_u16(&payload[59], g_config.takeup_pwm);
    write_u16(&payload[61], (uint16_t)g_config.ascent_correction_mm);
    *length = 63U;
}

static void bunker_config_payload(uint8_t *payload, uint8_t *length)
{
    payload[0] = LOT_RESULT_OK;
    write_u16(&payload[1], g_bunker_config.max_fill_x10);
    write_u16(&payload[3], g_bunker_config.target_fill_x10);
    write_u32(&payload[5], g_bunker_config.model_stroke_mm);
    write_u32(&payload[9], g_bunker_config.upper_height_mm);
    write_u32(&payload[13], g_bunker_config.upper_top_diameter_mm);
    write_u32(&payload[17], g_bunker_config.bunker_diameter_mm);
    write_u32(&payload[21], g_bunker_config.cylinder_height_mm);
    write_u32(&payload[25], g_bunker_config.lower_height_mm);
    write_u32(&payload[29], g_bunker_config.lower_bottom_diameter_mm);
    write_u32(&payload[33],
              g_bunker_config.sensor_height_above_base_mm);
    write_u32(&payload[37], g_bunker_config.density_x1000);
    write_u32(&payload[41], g_bunker_config.product_factor_x1000);
    *length = 45U;
}

static void protocol_dispatch(const uint8_t *frame, uint8_t length)
{
    const uint8_t command = frame[3];
    const uint8_t sequence = frame[4];
    const uint8_t payload_length = frame[5];
    const uint8_t *request = &frame[6];
    uint8_t response[LOT_PROTOCOL_MAX_PAYLOAD];
    uint8_t response_length = 1U;
    const uint32_t now = HAL_GetTick();

    (void)length;
    if (g_last_response_valid &&
        (command == g_last_request_command) &&
        (sequence == g_last_request_sequence))
    {
        protocol_send((uint8_t)(command | LOT_PROTOCOL_RESPONSE_FLAG),
                      sequence, g_last_response, g_last_response_length);
        return;
    }

    response[0] = LOT_RESULT_OK;

    switch ((lot_command_t)command)
    {
        case LOT_CMD_GET_STATUS:
            status_payload(response, &response_length);
            break;

        case LOT_CMD_GET_CALIBRATION_CAPTURE:
            if (payload_length != 0U)
            {
                response[0] = LOT_RESULT_BAD_LENGTH;
            }
            else
            {
                calibration_capture_payload(response, &response_length);
            }
            break;

        case LOT_CMD_GET_DIAGNOSTICS:
            if (payload_length != 0U)
            {
                response[0] = LOT_RESULT_BAD_LENGTH;
            }
            else
            {
                diagnostics_payload(response, &response_length);
            }
            break;

        case LOT_CMD_MOVE_UP:
        case LOT_CMD_MOVE_DOWN:
        case LOT_CMD_STOP:
        case LOT_CMD_PARK:
        case LOT_CMD_MEASURE_AUTO_ONCE:
            if (payload_length != 0U)
            {
                response[0] = LOT_RESULT_BAD_LENGTH;
            }
            else if (!start_command((lot_command_t)command, now))
            {
                response[0] = LOT_RESULT_BAD_STATE;
            }
            break;

        case LOT_CMD_SET_AUTO:
            if (payload_length != 1U)
            {
                response[0] = LOT_RESULT_BAD_LENGTH;
            }
            else if (request[0] > 1U)
            {
                response[0] = LOT_RESULT_BAD_VALUE;
            }
            else
            {
                g_status.auto_enabled = (request[0] != 0U);
                g_local_auto_enabled = false;
                g_next_auto_ms = now;
                if (request[0] == 0U)
                {
                    g_encoder_mismatch_count = 0U;
                }
            }
            break;

        case LOT_CMD_CLEAR_ERROR:
            if (payload_length != 0U)
            {
                response[0] = LOT_RESULT_BAD_LENGTH;
            }
            else
            {
                g_status.errors = LOT_ERROR_NONE;
                g_encoder_mismatch_count = 0U;
                enter_state(interlock_input_active() ?
                            LOT_STATE_INTERLOCK_WAIT :
                            LOT_STATE_READY, now);
            }
            break;

        case LOT_CMD_SET_MANUAL_PWM:
            if (payload_length != 2U)
            {
                response[0] = LOT_RESULT_BAD_LENGTH;
            }
            else
            {
                const uint16_t pwm = read_u16(request);
                if (pwm > 1000U)
                {
                    response[0] = LOT_RESULT_BAD_VALUE;
                }
                else
                {
                    g_manual_pwm = pwm;
                    if (g_status.state == LOT_STATE_MANUAL_UP)
                    {
                        motor_start(true, g_manual_pwm);
                    }
                    else if (g_status.state == LOT_STATE_MANUAL_DOWN)
                    {
                        if (ground_sensor_raw_active())
                        {
                            motor_stop();
                        }
                        else
                        {
                            motor_start(false, g_manual_pwm);
                        }
                    }
                }
            }
            break;

        case LOT_CMD_RESET_ENCODERS:
            if (payload_length != 0U)
            {
                response[0] = LOT_RESULT_BAD_LENGTH;
            }
            else if (g_status.motor_enabled)
            {
                response[0] = LOT_RESULT_BAD_STATE;
            }
            else
            {
                measurement_counters_reset(now, true);
            }
            break;

        case LOT_CMD_GET_CONFIG:
            config_payload(response, &response_length);
            break;

        case LOT_CMD_SET_CONFIG:
            if (payload_length != 62U)
            {
                response[0] = LOT_RESULT_BAD_LENGTH;
            }
            else
            {
                lot_config_t proposed = g_config;
                proposed.run_pwm = read_u16(&request[0]);
                proposed.park_pwm = read_u16(&request[2]);
                proposed.contact_timeout_ms = read_u16(&request[4]);
                proposed.stall_timeout_ms = read_u16(&request[6]);
                proposed.mismatch_mm = read_u16(&request[8]);
                proposed.park_release_ms = read_u16(&request[10]);
                proposed.park_test_ms = read_u16(&request[12]);
                proposed.park_test_pulses = read_u16(&request[14]);
                proposed.contact_speed_percent = read_u16(&request[16]);
                proposed.contact_ignore_ms = read_u16(&request[18]);
                proposed.slowdown_remaining_percent =
                    read_u16(&request[20]);
                proposed.slowdown_divisor_tenths =
                    read_u16(&request[22]);
                proposed.auto_interval_ms = read_u32(&request[24]);
                proposed.length_cal_x10000 = read_u32(&request[28]);
                proposed.motor_cal_x10000 = read_u32(&request[32]);
                proposed.length_cal_offset_tenth_mm =
                    (int32_t)read_u32(&request[36]);
                proposed.brake_comp_mm = read_u16(&request[40]);
                proposed.load_height_mm = read_u16(&request[42]);
                proposed.motor_encoder_enabled = read_u16(&request[44]);
                proposed.descent_correction_mm =
                    (int16_t)read_u16(&request[46]);
                proposed.error_measurement_limit = read_u16(&request[48]);
                proposed.motion_timeout_ms = read_u32(&request[50]);
                proposed.max_descent_mm = read_u32(&request[54]);
                proposed.takeup_pwm = read_u16(&request[58]);
                proposed.ascent_correction_mm =
                    (int16_t)read_u16(&request[60]);

                if ((proposed.run_pwm < MOTOR_MIN_ACTIVE_PWM) ||
                    (proposed.run_pwm > 1000U) ||
                    (proposed.park_pwm < MOTOR_MIN_ACTIVE_PWM) ||
                    (proposed.park_pwm > 1000U) ||
                    (proposed.contact_timeout_ms < 50U) ||
                    (proposed.stall_timeout_ms < 100U) ||
                    (proposed.park_release_ms < 100U) ||
                    (proposed.park_test_ms < 200U) ||
                    (proposed.park_test_pulses == 0U) ||
                    (proposed.contact_speed_percent == 0U) ||
                    (proposed.contact_speed_percent > 100U) ||
                    (proposed.contact_ignore_ms > 60000U) ||
                    (proposed.slowdown_remaining_percent == 0U) ||
                    (proposed.slowdown_remaining_percent > 100U) ||
                    (proposed.slowdown_divisor_tenths < 10U) ||
                    (proposed.auto_interval_ms < 1000UL) ||
                    (proposed.length_cal_x10000 < LENGTH_CAL_MIN) ||
                    (proposed.length_cal_x10000 > LENGTH_CAL_MAX) ||
                    (proposed.motor_cal_x10000 < LENGTH_CAL_MIN) ||
                    (proposed.motor_cal_x10000 > LENGTH_CAL_MAX) ||
                    (proposed.mismatch_mm > 10000U) ||
                    (proposed.length_cal_offset_tenth_mm <
                     LENGTH_CAL_OFFSET_MIN) ||
                    (proposed.length_cal_offset_tenth_mm >
                     LENGTH_CAL_OFFSET_MAX) ||
                    (proposed.brake_comp_mm > 10000U) ||
                    (proposed.load_height_mm > 10000U) ||
                    (proposed.motor_encoder_enabled > 1U) ||
                    (proposed.descent_correction_mm < -10000) ||
                    (proposed.descent_correction_mm > 10000) ||
                    (proposed.error_measurement_limit == 0U) ||
                    (proposed.error_measurement_limit > 1000U) ||
                    (proposed.motion_timeout_ms < 5000UL) ||
                    (proposed.motion_timeout_ms > 600000UL) ||
                    (proposed.max_descent_mm == 0UL) ||
                    (proposed.max_descent_mm > 100000UL) ||
                    (proposed.takeup_pwm < MOTOR_MIN_ACTIVE_PWM) ||
                    (proposed.takeup_pwm > 1000U) ||
                    (proposed.ascent_correction_mm < -10000) ||
                    (proposed.ascent_correction_mm > 10000))
                {
                    response[0] = LOT_RESULT_BAD_VALUE;
                }
                else if (memcmp(&proposed, &g_config, sizeof(g_config)) != 0)
                {
                    g_config = proposed;
                    if (g_config.mismatch_mm == 0U)
                    {
                        g_encoder_mismatch_count = 0U;
                    }
                    if (!config_save())
                    {
                        g_status.errors |= LOT_ERROR_CONFIG;
                        response[0] = LOT_RESULT_BAD_VALUE;
                    }
                }
            }
            break;

        case LOT_CMD_GET_BUNKER_CONFIG:
            bunker_config_payload(response, &response_length);
            break;

        case LOT_CMD_SET_BUNKER_CONFIG:
            if (payload_length != 44U)
            {
                response[0] = LOT_RESULT_BAD_LENGTH;
            }
            else
            {
                bunker_config_t proposed;

                proposed.max_fill_x10 = read_u16(&request[0]);
                proposed.target_fill_x10 = read_u16(&request[2]);
                proposed.model_stroke_mm = read_u32(&request[4]);
                proposed.upper_height_mm = read_u32(&request[8]);
                proposed.upper_top_diameter_mm = read_u32(&request[12]);
                proposed.bunker_diameter_mm = read_u32(&request[16]);
                proposed.cylinder_height_mm = read_u32(&request[20]);
                proposed.lower_height_mm = read_u32(&request[24]);
                proposed.lower_bottom_diameter_mm = read_u32(&request[28]);
                proposed.sensor_height_above_base_mm =
                    read_u32(&request[32]);
                proposed.density_x1000 = read_u32(&request[36]);
                proposed.product_factor_x1000 = read_u32(&request[40]);

                if (!bunker_config_values_valid(&proposed))
                {
                    response[0] = LOT_RESULT_BAD_VALUE;
                }
                else if (memcmp(&proposed, &g_bunker_config,
                                sizeof(g_bunker_config)) != 0)
                {
                    g_bunker_config = proposed;
                    if (!config_save())
                    {
                        g_status.errors |= LOT_ERROR_CONFIG;
                        response[0] = LOT_RESULT_BAD_VALUE;
                    }
                }
            }
            break;

        default:
            response[0] = LOT_RESULT_BAD_COMMAND;
            break;
    }

    g_last_request_command = command;
    g_last_request_sequence = sequence;
    g_last_response_length = response_length;
    (void)memcpy(g_last_response, response, response_length);
    g_last_response_valid = true;
    protocol_send((uint8_t)(command | LOT_PROTOCOL_RESPONSE_FLAG),
                  sequence, response, response_length);
}

static void protocol_consume(uint8_t value)
{
    uint16_t received_crc;
    uint16_t calculated_crc;

    if ((g_parser_length == 0U) && (value != LOT_PROTOCOL_SYNC_1))
    {
        return;
    }
    if ((g_parser_length == 1U) && (value != LOT_PROTOCOL_SYNC_2))
    {
        g_parser_length = (value == LOT_PROTOCOL_SYNC_1) ? 1U : 0U;
        return;
    }

    if (g_parser_length >= FRAME_MAX_SIZE)
    {
        g_parser_length = 0U;
        g_parser_expected = 0U;
        return;
    }

    g_parser[g_parser_length++] = value;
    if (g_parser_length == 6U)
    {
        if ((g_parser[2] != LOT_PROTOCOL_VERSION) ||
            (g_parser[5] > LOT_PROTOCOL_MAX_PAYLOAD))
        {
            g_parser_length = 0U;
            return;
        }
        g_parser_expected = (uint8_t)(g_parser[5] + 8U);
    }

    if ((g_parser_expected != 0U) &&
        (g_parser_length == g_parser_expected))
    {
        const uint8_t crc_index = (uint8_t)(g_parser_expected - 2U);
        received_crc = read_u16(&g_parser[crc_index]);
        calculated_crc = crc16_modbus(&g_parser[2],
                                      (uint8_t)(g_parser_expected - 4U));
        if (received_crc == calculated_crc)
        {
            protocol_dispatch(g_parser, g_parser_expected);
        }
        g_parser_length = 0U;
        g_parser_expected = 0U;
    }
}

static void protocol_send(uint8_t command, uint8_t sequence,
                          const uint8_t *payload, uint8_t length)
{
    uint16_t crc;

    if ((length > LOT_PROTOCOL_MAX_PAYLOAD) || g_tx_pending || g_tx_busy)
    {
        return;
    }

    g_tx_frame[0] = LOT_PROTOCOL_SYNC_1;
    g_tx_frame[1] = LOT_PROTOCOL_SYNC_2;
    g_tx_frame[2] = LOT_PROTOCOL_VERSION;
    g_tx_frame[3] = command;
    g_tx_frame[4] = sequence;
    g_tx_frame[5] = length;
    if ((length != 0U) && (payload != NULL))
    {
        (void)memcpy(&g_tx_frame[6], payload, length);
    }
    crc = crc16_modbus(&g_tx_frame[2], (uint8_t)(length + 4U));
    write_u16(&g_tx_frame[6U + length], crc);
    g_tx_length = (uint8_t)(length + 8U);
    g_tx_pending = true;
    g_tx_due_ms = HAL_GetTick() + TX_TURNAROUND_MS;
}

static void protocol_service(uint32_t now)
{
    if (g_tx_pending && !g_tx_busy &&
        ((int32_t)(now - g_tx_due_ms) >= 0))
    {
        g_tx_pending = false;
        g_tx_busy = true;
        HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_SET);
        if (HAL_UART_Transmit_IT(&g_uart1, g_tx_frame, g_tx_length) != HAL_OK)
        {
            HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
            g_tx_busy = false;
            g_uart_glitch_pending = 1U;
        }
    }
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

int main(void)
{
    HAL_Init();
    motor_outputs_safe_init();
    system_clock_config();
    config_load();
    g_manual_pwm = g_config.run_pwm;
    gpio_init();
    timers_init();

    __HAL_TIM_SET_COMPARE(&g_tim1_pwm, TIM_CHANNEL_1, 0U);
    if (HAL_TIM_PWM_Start(&g_tim1_pwm, TIM_CHANNEL_1) != HAL_OK)
    {
        for (;;)
        {
            __NOP();
        }
    }
    motor_pwm_pin_enable();

    uart_init();
    watchdog_init();

    (void)HAL_TIM_Encoder_Start(&g_tim3_encoder, TIM_CHANNEL_ALL);
    g_encoder_previous = (uint16_t)__HAL_TIM_GET_COUNTER(&g_tim3_encoder);
    g_last_encoder_ms = HAL_GetTick();
    g_fg_last_ms = HAL_GetTick();
    g_status.state = LOT_STATE_INIT;
    (void)HAL_UARTEx_ReceiveToIdle_DMA(&g_uart1, (uint8_t *)g_dma_rx,
                                       DMA_RX_SIZE);
    __HAL_DMA_DISABLE_IT(&g_dma_uart1_rx, DMA_IT_HT);

    for (;;)
    {
        const uint32_t now = HAL_GetTick();

        while (g_rx_read != g_rx_write)
        {
            const uint8_t value = g_rx_ring[g_rx_read];
            g_rx_read = (uint16_t)((g_rx_read + 1U) % RX_RING_SIZE);
            protocol_consume(value);
        }
        protocol_service(now);
        ground_sensor_led_mirror();
        if ((now - g_last_app_ms) >= APP_TICK_MS)
        {
            g_last_app_ms = now;
            app_tick(now);
        }
        state_led_update(now);
        busy_output_update();
        (void)HAL_IWDG_Refresh(&g_iwdg);
        __WFI();
    }
}

static void motor_outputs_safe_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(MOT_PWM_BLUE_PORT, MOT_PWM_BLUE_PIN, GPIO_PIN_RESET);
    gpio.Pin = MOT_PWM_BLUE_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MOT_PWM_BLUE_PORT, &gpio);

    HAL_GPIO_WritePin(MOT_DIR_WHITE_PORT, MOT_DIR_WHITE_PIN, GPIO_PIN_RESET);
    gpio.Pin = MOT_DIR_WHITE_PIN;
    HAL_GPIO_Init(MOT_DIR_WHITE_PORT, &gpio);

    HAL_GPIO_WritePin(BUSY_OUTPUT_PORT, BUSY_OUTPUT_PIN, GPIO_PIN_RESET);
    gpio.Pin = BUSY_OUTPUT_PIN;
    HAL_GPIO_Init(BUSY_OUTPUT_PORT, &gpio);
}

static void motor_pwm_pin_enable(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_TIM_SET_COMPARE(&g_tim1_pwm, TIM_CHANNEL_1, 0U);
    gpio.Pin = MOT_PWM_BLUE_PIN;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MOT_PWM_BLUE_PORT, &gpio);
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

    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
    __HAL_AFIO_REMAP_TIM3_PARTIAL();

    HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOT_DIR_WHITE_PORT, MOT_DIR_WHITE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STATE_LED_PORT, STATE_LED_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUSY_OUTPUT_PORT, BUSY_OUTPUT_PIN, GPIO_PIN_RESET);
    ground_sensor_led_mirror();

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
               GPIO_PIN_3 | GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = ENC_FA_WHITE_PIN | ENC_FB_GREEN_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ENC_FA_WHITE_PORT, &gpio);

    gpio.Pin = GROUND_SENSOR_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GROUND_SENSOR_PORT, &gpio);

    gpio.Pin = INTERLOCK_INPUT_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(INTERLOCK_INPUT_PORT, &gpio);

    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MOT_PWM_BLUE_PORT, &gpio);

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = RS485_DE_PIN | STATE_LED_PIN | BUSY_OUTPUT_PIN |
               MOT_DIR_WHITE_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = MOT_FG_YELLOW_PIN;
    gpio.Mode = GPIO_MODE_IT_RISING;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(MOT_FG_YELLOW_PORT, &gpio);

    gpio.Pin = STATUS_LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STATUS_LED_PORT, &gpio);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2U, 0U);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static void timers_init(void)
{
    TIM_OC_InitTypeDef pwm = {0};
    TIM_Encoder_InitTypeDef encoder = {0};
    TIM_MasterConfigTypeDef master = {0};

    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();

    g_tim1_pwm.Instance = TIM1;
    g_tim1_pwm.Init.Prescaler = 71U;
    g_tim1_pwm.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_tim1_pwm.Init.Period = 999U;
    g_tim1_pwm.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_tim1_pwm.Init.RepetitionCounter = 0U;
    if (HAL_TIM_PWM_Init(&g_tim1_pwm) != HAL_OK)
    {
        for (;;)
        {
            __NOP();
        }
    }
    pwm.OCMode = TIM_OCMODE_PWM1;
    pwm.Pulse = 0U;
    pwm.OCPolarity = TIM_OCPOLARITY_HIGH;
    pwm.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    pwm.OCFastMode = TIM_OCFAST_DISABLE;
    pwm.OCIdleState = TIM_OCIDLESTATE_RESET;
    pwm.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    (void)HAL_TIM_PWM_ConfigChannel(&g_tim1_pwm, &pwm, TIM_CHANNEL_1);

    g_tim3_encoder.Instance = TIM3;
    g_tim3_encoder.Init.Prescaler = 0U;
    g_tim3_encoder.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_tim3_encoder.Init.Period = 0xFFFFU;
    g_tim3_encoder.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    encoder.EncoderMode = TIM_ENCODERMODE_TI12;
    encoder.IC1Polarity = TIM_ICPOLARITY_RISING;
    encoder.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    encoder.IC1Prescaler = TIM_ICPSC_DIV1;
    encoder.IC1Filter = 6U;
    encoder.IC2Polarity = TIM_ICPOLARITY_FALLING;
    encoder.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    encoder.IC2Prescaler = TIM_ICPSC_DIV1;
    encoder.IC2Filter = 6U;
    if (HAL_TIM_Encoder_Init(&g_tim3_encoder, &encoder) != HAL_OK)
    {
        for (;;)
        {
            __NOP();
        }
    }
    master.MasterOutputTrigger = TIM_TRGO_RESET;
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    (void)HAL_TIMEx_MasterConfigSynchronization(&g_tim3_encoder, &master);
}

static void uart_init(void)
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
    (void)HAL_DMA_Init(&g_dma_uart1_rx);

    g_uart1.Instance = USART1;
    g_uart1.Init.BaudRate = 115200U;
    g_uart1.Init.WordLength = UART_WORDLENGTH_8B;
    g_uart1.Init.StopBits = UART_STOPBITS_1;
    g_uart1.Init.Parity = UART_PARITY_NONE;
    g_uart1.Init.Mode = UART_MODE_TX_RX;
    g_uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_uart1.Init.OverSampling = UART_OVERSAMPLING_16;
    (void)HAL_UART_Init(&g_uart1);
    __HAL_LINKDMA(&g_uart1, hdmarx, g_dma_uart1_rx);

    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 1U, 0U);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    HAL_NVIC_SetPriority(USART1_IRQn, 1U, 0U);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

static void watchdog_init(void)
{
    g_iwdg.Instance = IWDG;
    g_iwdg.Init.Prescaler = IWDG_PRESCALER_64;
    g_iwdg.Init.Reload = 2500U;
    (void)HAL_IWDG_Init(&g_iwdg);
}

static bool bunker_config_values_valid(const bunker_config_t *config)
{
    return (config->max_fill_x10 <= 1000U) &&
           (config->target_fill_x10 <= config->max_fill_x10) &&
           (config->model_stroke_mm > 0UL) &&
           (config->model_stroke_mm <= 100000UL) &&
           (config->upper_height_mm > 0UL) &&
           (config->upper_height_mm <= 100000UL) &&
           (config->upper_top_diameter_mm > 0UL) &&
           (config->bunker_diameter_mm > 0UL) &&
           (config->bunker_diameter_mm <= 100000UL) &&
           (config->upper_top_diameter_mm <=
            config->bunker_diameter_mm) &&
           (config->cylinder_height_mm > 0UL) &&
           (config->cylinder_height_mm <= 1000000UL) &&
           (config->lower_height_mm > 0UL) &&
           (config->lower_height_mm <= 100000UL) &&
           (config->lower_bottom_diameter_mm > 0UL) &&
           (config->lower_bottom_diameter_mm <=
            config->bunker_diameter_mm) &&
           (config->sensor_height_above_base_mm <=
            config->upper_height_mm) &&
           (config->density_x1000 <= 1000000UL) &&
           (config->product_factor_x1000 <= 1000000UL);
}

static bool config_values_valid(const lot_config_t *config)
{
    return (config->run_pwm >= MOTOR_MIN_ACTIVE_PWM) &&
           (config->run_pwm <= 1000U) &&
           (config->park_pwm >= MOTOR_MIN_ACTIVE_PWM) &&
           (config->park_pwm <= 1000U) &&
           (config->contact_timeout_ms >= 50U) &&
           (config->stall_timeout_ms >= 100U) &&
           (config->park_release_ms >= 100U) &&
           (config->park_test_ms >= 200U) &&
           (config->park_test_pulses != 0U) &&
           (config->contact_speed_percent != 0U) &&
           (config->contact_speed_percent <= 100U) &&
           (config->contact_ignore_ms <= 60000U) &&
           (config->slowdown_remaining_percent != 0U) &&
           (config->slowdown_remaining_percent <= 100U) &&
           (config->slowdown_divisor_tenths >= 10U) &&
           (config->auto_interval_ms >= 1000UL) &&
           (config->length_cal_x10000 >= LENGTH_CAL_MIN) &&
           (config->length_cal_x10000 <= LENGTH_CAL_MAX) &&
           (config->motor_cal_x10000 >= LENGTH_CAL_MIN) &&
           (config->motor_cal_x10000 <= LENGTH_CAL_MAX) &&
           (config->mismatch_mm <= 10000U) &&
           (config->length_cal_offset_tenth_mm >= LENGTH_CAL_OFFSET_MIN) &&
           (config->length_cal_offset_tenth_mm <= LENGTH_CAL_OFFSET_MAX) &&
           (config->brake_comp_mm <= 10000U) &&
           (config->load_height_mm <= 10000U) &&
           (config->motor_encoder_enabled <= 1U) &&
           (config->descent_correction_mm >= -10000) &&
           (config->descent_correction_mm <= 10000) &&
           (config->error_measurement_limit > 0U) &&
           (config->error_measurement_limit <= 1000U) &&
           (config->motion_timeout_ms >= 5000UL) &&
           (config->motion_timeout_ms <= 600000UL) &&
           (config->max_descent_mm > 0UL) &&
           (config->max_descent_mm <= 100000UL) &&
           (config->takeup_pwm >= MOTOR_MIN_ACTIVE_PWM) &&
           (config->takeup_pwm <= 1000U) &&
           (config->ascent_correction_mm >= -10000) &&
           (config->ascent_correction_mm <= 10000);
}

static void config_migrate_v14(const lot_config_v14_t *previous)
{
    (void)memcpy(&g_config, previous, sizeof(*previous));
    g_config.takeup_pwm = 945U;
    g_config.ascent_correction_mm = 0;
}

static void config_migrate_v13(const lot_config_v13_t *previous)
{
    (void)memcpy(&g_config, previous, sizeof(*previous));
    g_config.max_descent_mm = 1000UL;
}

static void config_migrate_v12(const lot_config_v12_t *previous)
{
    (void)memcpy(&g_config, previous, sizeof(*previous));
    g_config.error_measurement_limit = 2U;
    g_config.max_descent_mm = 1000UL;
}

static void config_migrate_v7(const lot_config_v7_t *previous)
{
    g_config.run_pwm = previous->run_pwm;
    g_config.park_pwm = previous->park_pwm;
    g_config.contact_timeout_ms = previous->contact_timeout_ms;
    g_config.stall_timeout_ms = previous->stall_timeout_ms;
    g_config.mismatch_mm = previous->mismatch_mm;
    g_config.park_release_ms = previous->park_release_ms;
    g_config.park_test_ms = previous->park_test_ms;
    g_config.park_test_pulses = previous->park_test_pulses;
    g_config.contact_speed_percent = 80U;
    g_config.slowdown_remaining_percent =
        previous->slowdown_remaining_percent;
    g_config.slowdown_divisor_tenths =
        previous->slowdown_divisor_tenths;
    g_config.manual_timeout_ms = previous->manual_timeout_ms;
    g_config.motion_timeout_ms = previous->motion_timeout_ms;
    g_config.auto_interval_ms = previous->auto_interval_ms;
    g_config.length_cal_x10000 = previous->length_cal_x10000;
    g_config.motor_cal_x10000 = previous->motor_cal_x10000;
    g_config.length_cal_offset_tenth_mm =
        previous->length_cal_offset_tenth_mm;
    g_config.contact_ignore_ms = 1000U;
    g_config.brake_comp_mm = 0U;
    g_config.load_height_mm = 110U;
    g_config.motor_encoder_enabled = 0U;
    g_config.descent_correction_mm = 0;
    g_config.error_measurement_limit = 2U;
}

static void config_migrate_v8(const lot_config_v8_t *previous)
{
    g_config.run_pwm = previous->run_pwm;
    g_config.park_pwm = previous->park_pwm;
    g_config.contact_timeout_ms = previous->contact_timeout_ms;
    g_config.stall_timeout_ms = previous->stall_timeout_ms;
    g_config.mismatch_mm = previous->mismatch_mm;
    g_config.park_release_ms = previous->park_release_ms;
    g_config.park_test_ms = previous->park_test_ms;
    g_config.park_test_pulses = previous->park_test_pulses;
    g_config.contact_speed_percent = previous->contact_speed_percent;
    g_config.slowdown_remaining_percent =
        previous->slowdown_remaining_percent;
    g_config.slowdown_divisor_tenths =
        previous->slowdown_divisor_tenths;
    g_config.manual_timeout_ms = previous->manual_timeout_ms;
    g_config.motion_timeout_ms = previous->motion_timeout_ms;
    g_config.auto_interval_ms = previous->auto_interval_ms;
    g_config.length_cal_x10000 = previous->length_cal_x10000;
    g_config.motor_cal_x10000 = previous->motor_cal_x10000;
    g_config.length_cal_offset_tenth_mm =
        previous->length_cal_offset_tenth_mm;
    g_config.contact_ignore_ms = previous->contact_ignore_ms;
    g_config.brake_comp_mm = 0U;
    g_config.load_height_mm = 110U;
    g_config.motor_encoder_enabled = 0U;
    g_config.descent_correction_mm = 0;
    g_config.error_measurement_limit = 2U;
}

static void config_migrate_v9(const lot_config_v9_t *previous)
{
    g_config.run_pwm = previous->run_pwm;
    g_config.park_pwm = previous->park_pwm;
    g_config.contact_timeout_ms = previous->contact_timeout_ms;
    g_config.stall_timeout_ms = previous->stall_timeout_ms;
    g_config.mismatch_mm = previous->mismatch_mm;
    g_config.park_release_ms = previous->park_release_ms;
    g_config.park_test_ms = previous->park_test_ms;
    g_config.park_test_pulses = previous->park_test_pulses;
    g_config.contact_speed_percent = previous->contact_speed_percent;
    g_config.slowdown_remaining_percent =
        previous->slowdown_remaining_percent;
    g_config.slowdown_divisor_tenths =
        previous->slowdown_divisor_tenths;
    g_config.manual_timeout_ms = previous->manual_timeout_ms;
    g_config.motion_timeout_ms = previous->motion_timeout_ms;
    g_config.auto_interval_ms = previous->auto_interval_ms;
    g_config.length_cal_x10000 = previous->length_cal_x10000;
    g_config.motor_cal_x10000 = previous->motor_cal_x10000;
    g_config.length_cal_offset_tenth_mm =
        previous->length_cal_offset_tenth_mm;
    g_config.contact_ignore_ms = previous->contact_ignore_ms;
    g_config.brake_comp_mm = previous->brake_comp_mm;
    g_config.load_height_mm = previous->load_height_mm;
    g_config.motor_encoder_enabled = 0U;
    g_config.descent_correction_mm = 0;
    g_config.error_measurement_limit = 2U;
}

static void config_migrate_v10(const lot_config_v10_t *previous)
{
    g_config.run_pwm = previous->run_pwm;
    g_config.park_pwm = previous->park_pwm;
    g_config.contact_timeout_ms = previous->contact_timeout_ms;
    g_config.stall_timeout_ms = previous->stall_timeout_ms;
    g_config.mismatch_mm = previous->mismatch_mm;
    g_config.park_release_ms = previous->park_release_ms;
    g_config.park_test_ms = previous->park_test_ms;
    g_config.park_test_pulses = previous->park_test_pulses;
    g_config.contact_speed_percent = previous->contact_speed_percent;
    g_config.slowdown_remaining_percent =
        previous->slowdown_remaining_percent;
    g_config.slowdown_divisor_tenths =
        previous->slowdown_divisor_tenths;
    g_config.manual_timeout_ms = previous->manual_timeout_ms;
    g_config.motion_timeout_ms = previous->motion_timeout_ms;
    g_config.auto_interval_ms = previous->auto_interval_ms;
    g_config.length_cal_x10000 = previous->length_cal_x10000;
    g_config.motor_cal_x10000 = previous->motor_cal_x10000;
    g_config.length_cal_offset_tenth_mm =
        previous->length_cal_offset_tenth_mm;
    g_config.contact_ignore_ms = previous->contact_ignore_ms;
    g_config.brake_comp_mm = previous->brake_comp_mm;
    g_config.load_height_mm = previous->load_height_mm;
    g_config.motor_encoder_enabled = previous->motor_encoder_enabled;
    g_config.descent_correction_mm = 0;
    g_config.error_measurement_limit = 2U;
}

static void config_load(void)
{
    const lot_config_t defaults = g_config;
    const bunker_config_t bunker_defaults = g_bunker_config;
    config_record_t record;
    config_record_v14_t record_v14;
    config_record_v13_t record_v13;
    config_record_v12_t record_v12;
    config_record_v11_t record_v11;
    config_record_v10_t record_v10;
    config_record_v9_t record_v9;
    config_record_v8_t record_v8;
    config_record_v7_t record_v7;
    config_record_v5_t record_v5;
    const uint8_t crc_length =
        (uint8_t)(sizeof(config_record_t) - sizeof(record.crc));
    const uint8_t crc_length_v14 =
        (uint8_t)(sizeof(config_record_v14_t) - sizeof(record_v14.crc));
    const uint8_t crc_length_v13 =
        (uint8_t)(sizeof(config_record_v13_t) - sizeof(record_v13.crc));
    const uint8_t crc_length_v12 =
        (uint8_t)(sizeof(config_record_v12_t) - sizeof(record_v12.crc));
    const uint8_t crc_length_v11 =
        (uint8_t)(sizeof(config_record_v11_t) - sizeof(record_v11.crc));
    const uint8_t crc_length_v10 =
        (uint8_t)(sizeof(config_record_v10_t) - sizeof(record_v10.crc));
    const uint8_t crc_length_v9 =
        (uint8_t)(sizeof(config_record_v9_t) - sizeof(record_v9.crc));
    const uint8_t crc_length_v8 =
        (uint8_t)(sizeof(config_record_v8_t) - sizeof(record_v8.crc));
    const uint8_t crc_length_v7 =
        (uint8_t)(sizeof(config_record_v7_t) - sizeof(record_v7.crc));
    const uint8_t crc_length_v5 =
        (uint8_t)(sizeof(config_record_v5_t) - sizeof(record_v5.crc));

    (void)memcpy(&record, (const void *)CONFIG_FLASH_ADDRESS,
                 sizeof(record));
    if ((record.magic == CONFIG_MAGIC) &&
        (record.version == CONFIG_VERSION) &&
        (record.crc == crc16_modbus((const uint8_t *)&record, crc_length)) &&
        config_values_valid(&record.config) &&
        bunker_config_values_valid(&record.bunker))
    {
        g_config = record.config;
        g_bunker_config = record.bunker;
        return;
    }

    (void)memcpy(&record_v14, (const void *)CONFIG_FLASH_ADDRESS,
                 sizeof(record_v14));
    if ((record_v14.magic == CONFIG_MAGIC) &&
        (record_v14.version == 14U) &&
        (record_v14.crc ==
         crc16_modbus((const uint8_t *)&record_v14, crc_length_v14)))
    {
        config_migrate_v14(&record_v14.config);
        if (config_values_valid(&g_config) &&
            bunker_config_values_valid(&record_v14.bunker))
        {
            g_bunker_config = record_v14.bunker;
            if (!config_save())
            {
                g_status.errors |= LOT_ERROR_CONFIG;
            }
            return;
        }
        g_config = defaults;
    }

    (void)memcpy(&record_v13, (const void *)CONFIG_FLASH_ADDRESS,
                 sizeof(record_v13));
    if ((record_v13.magic == CONFIG_MAGIC) &&
        (record_v13.version == 13U) &&
        (record_v13.crc ==
         crc16_modbus((const uint8_t *)&record_v13, crc_length_v13)))
    {
        config_migrate_v13(&record_v13.config);
        if (config_values_valid(&g_config) &&
            bunker_config_values_valid(&record_v13.bunker))
        {
            g_bunker_config = record_v13.bunker;
            return;
        }
        g_config = defaults;
    }

    (void)memcpy(&record_v12, (const void *)CONFIG_FLASH_ADDRESS,
                 sizeof(record_v12));
    if ((record_v12.magic == CONFIG_MAGIC) &&
        (record_v12.version == 12U) &&
        (record_v12.crc ==
         crc16_modbus((const uint8_t *)&record_v12, crc_length_v12)))
    {
        config_migrate_v12(&record_v12.config);
        if (config_values_valid(&g_config) &&
            bunker_config_values_valid(&record_v12.bunker))
        {
            g_bunker_config = record_v12.bunker;
            return;
        }
        g_config = defaults;
    }

    (void)memcpy(&record_v11, (const void *)CONFIG_FLASH_ADDRESS,
                 sizeof(record_v11));
    if ((record_v11.magic == CONFIG_MAGIC) &&
        (record_v11.version == 11U) &&
        (record_v11.crc ==
         crc16_modbus((const uint8_t *)&record_v11, crc_length_v11)))
    {
        config_migrate_v12(&record_v11.config);
        if (config_values_valid(&g_config))
        {
            g_bunker_config = bunker_defaults;
            return;
        }
        g_config = defaults;
    }

    (void)memcpy(&record_v10, (const void *)CONFIG_FLASH_ADDRESS,
                 sizeof(record_v10));
    if ((record_v10.magic == CONFIG_MAGIC) &&
        (record_v10.version == 10U) &&
        (record_v10.crc ==
         crc16_modbus((const uint8_t *)&record_v10, crc_length_v10)))
    {
        config_migrate_v10(&record_v10.config);
        if (config_values_valid(&g_config))
        {
            return;
        }
        g_config = defaults;
    }

    (void)memcpy(&record_v9, (const void *)CONFIG_FLASH_ADDRESS,
                 sizeof(record_v9));
    if ((record_v9.magic == CONFIG_MAGIC) &&
        (record_v9.version == 9U) &&
        (record_v9.crc ==
         crc16_modbus((const uint8_t *)&record_v9, crc_length_v9)))
    {
        config_migrate_v9(&record_v9.config);
        if (config_values_valid(&g_config))
        {
            return;
        }
        g_config = defaults;
    }

    (void)memcpy(&record_v8, (const void *)CONFIG_FLASH_ADDRESS,
                 sizeof(record_v8));
    if ((record_v8.magic == CONFIG_MAGIC) &&
        (record_v8.version == 8U) &&
        (record_v8.crc ==
         crc16_modbus((const uint8_t *)&record_v8, crc_length_v8)))
    {
        config_migrate_v8(&record_v8.config);
        if (config_values_valid(&g_config))
        {
            return;
        }
        g_config = defaults;
    }

    (void)memcpy(&record_v7, (const void *)CONFIG_FLASH_ADDRESS,
                 sizeof(record_v7));
    if ((record_v7.magic == CONFIG_MAGIC) &&
        ((record_v7.version == 7U) || (record_v7.version == 6U)) &&
        (record_v7.crc ==
         crc16_modbus((const uint8_t *)&record_v7, crc_length_v7)))
    {
        config_migrate_v7(&record_v7.config);
        if (record_v7.version == 6U)
        {
            g_config.mismatch_mm = 100U;
        }
        if (config_values_valid(&g_config))
        {
            return;
        }
        g_config = defaults;
    }

    (void)memcpy(&record_v5, (const void *)CONFIG_FLASH_ADDRESS,
                 sizeof(record_v5));
    if ((record_v5.magic == CONFIG_MAGIC) &&
        (record_v5.version == 5U) &&
        (record_v5.crc ==
         crc16_modbus((const uint8_t *)&record_v5, crc_length_v5)))
    {
        lot_config_v7_t previous = {0};

        (void)memcpy(&previous, record_v5.config_data,
                     sizeof(record_v5.config_data));
        previous.length_cal_offset_tenth_mm = 0;
        config_migrate_v7(&previous);
        g_config.mismatch_mm = 100U;
        if (config_values_valid(&g_config))
        {
            return;
        }
    }
    g_config = defaults;
    g_bunker_config = bunker_defaults;
}

static bool config_save(void)
{
    config_record_t record;
    uint16_t words[sizeof(config_record_t) / 2U];
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;
    uint32_t index;
    HAL_StatusTypeDef result;

    record.magic = CONFIG_MAGIC;
    record.version = CONFIG_VERSION;
    record.config = g_config;
    record.bunker = g_bunker_config;
    record.crc = crc16_modbus(
        (const uint8_t *)&record,
        (uint8_t)(sizeof(config_record_t) - sizeof(record.crc)));
    (void)memcpy(words, &record, sizeof(record));

    result = HAL_FLASH_Unlock();
    if (result == HAL_OK)
    {
        erase.TypeErase = FLASH_TYPEERASE_PAGES;
        erase.PageAddress = CONFIG_FLASH_ADDRESS;
        erase.NbPages = 1U;
        result = HAL_FLASHEx_Erase(&erase, &page_error);
    }
    for (index = 0U;
         (result == HAL_OK) && (index < (sizeof(words) / sizeof(words[0])));
         ++index)
    {
        result = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_HALFWORD,
            CONFIG_FLASH_ADDRESS + (index * sizeof(uint16_t)),
            words[index]);
    }
    (void)HAL_FLASH_Lock();
    return result == HAL_OK;
}
