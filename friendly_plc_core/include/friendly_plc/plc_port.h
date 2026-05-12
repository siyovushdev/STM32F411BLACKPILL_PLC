#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t di_count;
    uint16_t do_count;
    uint16_t ai_count;
    uint16_t ao_count;
    uint16_t hsc_count;
    uint16_t encoder_count;
} PlcPortHwInfo;

PlcPortHwInfo plc_port_get_hw_info(void);

bool plc_port_read_di(uint16_t ch);
void plc_port_write_do(uint16_t ch, bool value);

int32_t plc_port_read_ai_mv(uint16_t ch);
void plc_port_write_ao_percent(uint16_t ch, float percent);

int32_t plc_port_read_hsc(uint16_t ch);
int32_t plc_port_read_encoder(uint16_t ch);

uint32_t plc_port_now_ms(void);

void plc_port_feed_watchdog(void);
void plc_port_reset_outputs(void);
void plc_port_stop_pwm(void);
void plc_port_set_safe_outputs(void);

#ifdef FRIENDLY_PLC_ENABLE_TEST_API
bool plc_port_test_get_do(uint16_t ch);
void plc_port_test_set_di(uint16_t ch, bool value);
void plc_port_test_set_ai_mv(uint16_t ch, int32_t value);
float plc_port_test_get_ao_percent(uint16_t ch);
#endif

#ifdef FRIENDLY_PLC_ENABLE_TEST_API
void plc_port_test_set_hsc(uint16_t ch, int32_t value);
void plc_port_test_set_encoder(uint16_t ch, int32_t value);
#endif

#ifdef __cplusplus
}
#endif