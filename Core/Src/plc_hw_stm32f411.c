#include "plc_hw_stm32f411.h"

#include "plc_port_stm32.h"
#include "main.h"
#include "ad7606.h"
#include "dac8568.h"
#include "plc_ai.h"
#include "plc_ao.h"

#include <stdbool.h>
#include <stdint.h>
#include "plc_hw_spi.h"

extern IWDG_HandleTypeDef hiwdg;
extern SPI_HandleTypeDef hspi1;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    bool inverted;
} PlcGpioInput;

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    bool inverted;
} PlcGpioOutput;

#define ARRAY_LEN(a) ((uint16_t)(sizeof(a) / sizeof((a)[0])))
#define PLC_AI_ENABLED 0
#define PLC_AO_ENABLED 0


#if PLC_AI_ENABLED
static AD7606_Handle s_ad7606 = {
        .hspi = &hspi1,
        .CS_Port = ADC_CS_GPIO_Port,
        .CS_Pin = ADC_CS_Pin,
        .RST_Port = ADC_RST_GPIO_Port,
        .RST_Pin = ADC_RST_Pin,
        .BUSY_Port = ADC_BUSY_GPIO_Port,
        .BUSY_Pin = ADC_BUSY_Pin,
        .CVA_Port = ADC_CONVST_GPIO_Port,
        .CVA_Pin = ADC_CONVST_Pin,
        .CVB_Port = ADC_CONVST_GPIO_Port,
        .CVB_Pin = ADC_CONVST_Pin,
        .t_conv_timeout_ms = 10u,
        .spi_timeout_ms = 10u,
};

static PlcAi s_ai;
static uint32_t s_ai_last_tick_ms = UINT32_MAX;
#endif

#if PLC_AO_ENABLED
static PlcAo s_ao;
#endif

static bool app_read_di(uint16_t ch, void* user)
{
    (void)user;

    if (ch >= 16u) {
        return false;
    }

    uint16_t di_bits = 0;
    if (!plc_hw_spi_read_di(&di_bits)) {
        return false;
    }

    return ((di_bits >> ch) & 0x0001u) != 0u;
}

static void board_write_do_raw(uint16_t ch, bool logical_value)
{
    static uint16_t do_bits = 0;

    if (ch >= 16u) {
        return;
    }

    if (logical_value) {
        do_bits |= (uint16_t)(1u << ch);
    } else {
        do_bits &= (uint16_t)~(1u << ch);
    }

    (void)plc_hw_spi_write_do(do_bits);
}

static void app_write_do(uint16_t ch, bool value, void* user)
{
    (void)user;
    board_write_do_raw(ch, value);
}

static int32_t app_read_ai_mv(uint16_t ch, void* user)
{
    (void)user;

#if PLC_AI_ENABLED
    if (ch >= PLC_AI_CH_COUNT) return 0;

    const uint32_t now = HAL_GetTick();
    if (s_ai_last_tick_ms != now) {
        s_ai_last_tick_ms = now;
        (void)PlcAi_Read(&s_ai);
    }

    if (!PlcAi_IsValid(&s_ai, (uint8_t)ch)) return 0;

    float volts = PlcAi_GetValue(&s_ai, (uint8_t)ch);
    if (volts < 0.0f) volts = 0.0f;
    if (volts > 10.0f) volts = 10.0f;

    return (int32_t)(volts * 1000.0f + 0.5f);
#else
    (void)ch;
    return 0;
#endif
}

static void app_write_ao_percent(uint16_t ch, float percent, void* user)
{
    (void)user;

#if PLC_AO_ENABLED
    if (ch >= 8u) return;
    (void)PlcAo_SetPercent(&s_ao, (uint8_t)ch, percent);
#else
    (void)ch;
    (void)percent;
#endif
}

static int32_t app_read_hsc(uint16_t ch, void* user)
{
    (void)user;

    switch (ch) {
        case 0: return (int32_t)__HAL_TIM_GET_COUNTER(&htim1);
        case 1: return (int32_t)__HAL_TIM_GET_COUNTER(&htim3);
        case 2: return (int32_t)__HAL_TIM_GET_COUNTER(&htim5);
        default: return 0;
    }
}

static int32_t app_read_encoder(uint16_t ch, void* user)
{
    (void)user;

    switch (ch) {
        case 0: return (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
        case 1: return (int32_t)__HAL_TIM_GET_COUNTER(&htim4);
        default: return 0;
    }
}

static uint32_t app_now_ms(void* user)
{
    (void)user;
    return HAL_GetTick();
}

static void app_feed_watchdog(void* user)
{
    (void)user;
    HAL_IWDG_Refresh(&hiwdg);
}

static void app_reset_outputs(void* user)
{
    (void)user;
    plc_hw_stm32f411_reset_outputs();
}

static void app_set_safe_outputs(void* user)
{
    (void)user;
    plc_hw_stm32f411_set_safe_outputs();
}


void plc_hw_stm32f411_reset_outputs(void)
{
    (void)plc_hw_spi_write_do(0x0000u);

#if PLC_AO_ENABLED
    (void)PlcAo_ResetAllToZeroVolt(&s_ao);
#endif
}

bool plc_hw_stm32f411_init(void)
{
//    plc_hw_stm32f411_reset_outputs();

#if PLC_AI_ENABLED
    if (AD7606_Init(&s_ad7606) != HAL_OK) return false;
    PlcAi_Init(&s_ai, &s_ad7606);
#endif

#if PLC_AO_ENABLED
    PlcAo_Init(&s_ao);
    if (DAC8568_Init() != HAL_OK) return false;
    (void)PlcAo_ResetAllToZeroVolt(&s_ao);
#endif

    if (HAL_TIM_Base_Start(&htim1) != HAL_OK) return false;
    if (HAL_TIM_Base_Start(&htim3) != HAL_OK) return false;
    if (HAL_TIM_Base_Start(&htim5) != HAL_OK) return false;

    if (HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL) != HAL_OK) return false;
    if (HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL) != HAL_OK) return false;

    PlcPortStm32Config cfg = {
            .hw = {
                    .di_count = 16,
                    .do_count = 16,
                    .ai_count = PLC_AI_ENABLED ? PLC_AI_CH_COUNT : 0,
                    .ao_count = PLC_AO_ENABLED ? 8 : 0,
                    .hsc_count = 3,
                    .encoder_count = 2,
            },
            .read_di = app_read_di,
            .write_do = app_write_do,
            .read_ai_mv = app_read_ai_mv,
            .write_ao_percent = app_write_ao_percent,
            .read_hsc = app_read_hsc,
            .read_encoder = app_read_encoder,
            .now_ms = app_now_ms,
            .feed_watchdog = app_feed_watchdog,
            .reset_outputs = app_reset_outputs,
            .user = NULL,
            .set_safe_outputs = app_set_safe_outputs,
    };

    return plc_port_stm32_init(&cfg);
}


void plc_hw_stm32f411_set_safe_outputs(void)
{
    plc_hw_stm32f411_reset_outputs();
}