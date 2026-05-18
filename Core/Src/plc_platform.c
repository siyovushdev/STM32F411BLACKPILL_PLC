#include "plc_platform.h"

#include "main.h"
#include "plc_hw_stm32f411.h"

#include "friendly_plc/plc.h"
#include "plc_hw_spi.h"

extern IWDG_HandleTypeDef hiwdg;

bool plc_platform_init(void)
{
    if (!plc_hw_spi_init()) {
        return false;
    }

    if (!plc_hw_stm32f411_init()) {
        return false;
    }

    plc_mem_init();

    return true;
}

void plc_platform_scan_cycle(void)
{
    plc_tick(HAL_GetTick());
}

uint32_t plc_platform_now_ms(void)
{
    return HAL_GetTick();
}

void plc_platform_feed_watchdog(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}

void plc_platform_reset_outputs(void)
{
    plc_hw_stm32f411_reset_outputs();
}

void plc_platform_on_diag_tick(void)
{
    plc_platform_feed_watchdog();
}
