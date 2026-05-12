#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool plc_hw_stm32f411_init(void);
void plc_hw_stm32f411_reset_outputs(void);
void plc_hw_stm32f411_set_safe_outputs(void);
#ifdef __cplusplus
}
#endif
