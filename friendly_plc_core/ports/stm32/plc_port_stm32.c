#include "plc_port_stm32.h"
#include "friendly_plc/plc_log.h"

#include <string.h>

static PlcPortStm32Config s_cfg;
static bool s_inited = false;

bool plc_port_stm32_init(const PlcPortStm32Config* config)
{
    if (!config) {
        return false;
    }

    memset(&s_cfg, 0, sizeof(s_cfg));
    s_cfg = *config;

    s_inited = true;

    if (s_cfg.reset_outputs) {
        s_cfg.reset_outputs(s_cfg.user);
    }

    return true;
}

PlcPortHwInfo plc_port_get_hw_info(void)
{
    if (!s_inited) {
        PlcPortHwInfo empty = {0};
        return empty;
    }

    return s_cfg.hw;
}

bool plc_port_read_di(uint16_t ch)
{
//    PLC_LOGI("STM32_PORT", "read_di ch=%u", (unsigned)ch);
//    if (!s_inited || !s_cfg.read_di || ch >= s_cfg.hw.di_count) {
//        return false;
//    }

    return s_cfg.read_di(ch, s_cfg.user);
}

void plc_port_write_do(uint16_t channel, bool value)
{
    static uint8_t inited[64];
    static uint8_t last[64];

    if (channel < 64) {
        if (!inited[channel] || last[channel] != (uint8_t)value) {
            inited[channel] = 1;
            last[channel] = (uint8_t)value;

            PLC_LOGI("STM32_PORT",
                     "write_do ch=%u value=%u",
                     (unsigned)channel,
                     (unsigned)value);
        }
    }
//    PLC_LOGI("STM32_PORT", "write_do ch=%u value=%u",
//             (unsigned)ch,
//             (unsigned)value);
    if (!s_inited || !s_cfg.write_do || channel >= s_cfg.hw.do_count) {
        return;
    }

    s_cfg.write_do(channel, value, s_cfg.user);
}

int32_t plc_port_read_ai_mv(uint16_t ch)
{
    if (!s_inited || !s_cfg.read_ai_mv || ch >= s_cfg.hw.ai_count) {
        return 0;
    }

    return s_cfg.read_ai_mv(ch, s_cfg.user);
}

void plc_port_write_ao_percent(uint16_t ch, float percent)
{
    if (!s_inited || !s_cfg.write_ao_percent || ch >= s_cfg.hw.ao_count) {
        return;
    }

    if (percent < 0.0f) {
        percent = 0.0f;
    }

    if (percent > 100.0f) {
        percent = 100.0f;
    }

    s_cfg.write_ao_percent(ch, percent, s_cfg.user);
}

int32_t plc_port_read_hsc(uint16_t ch)
{
    if (!s_inited || !s_cfg.read_hsc || ch >= s_cfg.hw.hsc_count) {
        return 0;
    }

    return s_cfg.read_hsc(ch, s_cfg.user);
}

int32_t plc_port_read_encoder(uint16_t ch)
{
    if (!s_inited || !s_cfg.read_encoder || ch >= s_cfg.hw.encoder_count) {
        return 0;
    }

    return s_cfg.read_encoder(ch, s_cfg.user);
}

uint32_t plc_port_now_ms(void)
{
    if (!s_inited || !s_cfg.now_ms) {
        return 0;
    }

    return s_cfg.now_ms(s_cfg.user);
}

void plc_port_feed_watchdog(void)
{
    if (!s_inited || !s_cfg.feed_watchdog) {
        return;
    }

    s_cfg.feed_watchdog(s_cfg.user);
}

void plc_port_reset_outputs(void)
{
    if (!s_inited || !s_cfg.reset_outputs) {
        return;
    }

    s_cfg.reset_outputs(s_cfg.user);
}

void plc_port_stop_pwm(void)
{
    if (!s_inited || !s_cfg.stop_pwm) {
        return;
    }

    s_cfg.stop_pwm(s_cfg.user);
}

void plc_port_set_safe_outputs(void)
{
    if (!s_inited) {
        return;
    }

    if (s_cfg.set_safe_outputs) {
        s_cfg.set_safe_outputs(s_cfg.user);
        return;
    }

    if (s_cfg.reset_outputs) {
        s_cfg.reset_outputs(s_cfg.user);
    }
}
