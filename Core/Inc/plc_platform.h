#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool plc_platform_init(void);
void plc_platform_scan_cycle(void);
uint32_t plc_platform_now_ms(void);
void plc_platform_feed_watchdog(void);
void plc_platform_reset_outputs(void);

void plc_platform_on_diag_tick(void);

/* Optional project overrides. Default weak implementations are safe no-op. */
void plc_platform_user_scan_cycle(void);
void plc_platform_user_diag_tick(void);

#ifdef __cplusplus
}
#endif
