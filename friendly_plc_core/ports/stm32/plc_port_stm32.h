#pragma once

#include "friendly_plc/plc_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    PlcPortHwInfo hw;

    bool    (*read_di)(uint16_t ch, void* user);
    void    (*write_do)(uint16_t ch, bool value, void* user);

    int32_t (*read_ai_mv)(uint16_t ch, void* user);
    void    (*write_ao_percent)(uint16_t ch, float percent, void* user);

    int32_t (*read_hsc)(uint16_t ch, void* user);
    int32_t (*read_encoder)(uint16_t ch, void* user);

    uint32_t (*now_ms)(void* user);
    void     (*feed_watchdog)(void* user);
    void     (*reset_outputs)(void* user);
    void     (*stop_pwm)(void* user);
    void     (*set_safe_outputs)(void* user);

    void* user;
} PlcPortStm32Config;

bool plc_port_stm32_init(const PlcPortStm32Config* config);

#ifdef __cplusplus
}
#endif