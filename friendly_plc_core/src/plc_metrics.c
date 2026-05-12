#include <stdint.h>

uint32_t g_scan_cnt = 0;
uint64_t g_scan_us_accum = 0;
uint32_t g_scan_us_max = 0;
uint32_t g_scan_long_steps = 0;

#define SCAN_STAT_WINDOW_CYCLES 1000u

void plc_metrics_on_cycle_us(uint32_t execUs, uint32_t periodMs)
{
    g_scan_cnt++;
    g_scan_us_accum += (uint64_t)execUs;
    if (execUs > g_scan_us_max) g_scan_us_max = execUs;

    if (execUs > (periodMs * 1000u)) g_scan_long_steps++;

    if (g_scan_cnt >= SCAN_STAT_WINDOW_CYCLES) {
        g_scan_cnt = 0;
        g_scan_us_accum = 0;
        g_scan_us_max = 0;
    }
}

uint32_t g_work_cnt = 0;
uint64_t g_work_us_accum = 0;
uint32_t g_work_us_max = 0;

uint32_t g_cycle_cnt = 0;
uint64_t g_cycle_ms_accum = 0;
uint32_t g_cycle_ms_max = 0;

#define STAT_WINDOW 1000u

void plc_metrics_on_work_us(uint32_t workUs)
{
    g_work_cnt++;
    g_work_us_accum += (uint64_t)workUs;
    if (workUs > g_work_us_max) g_work_us_max = workUs;

    if (g_work_cnt >= STAT_WINDOW) {
        g_work_cnt = 0;
        g_work_us_accum = 0;
        g_work_us_max = 0;
    }
}

void plc_metrics_on_cycle_ms(uint32_t realCycleMs)
{
    if (realCycleMs == 0) return;

    g_cycle_cnt++;
    g_cycle_ms_accum += (uint64_t)realCycleMs;
    if (realCycleMs > g_cycle_ms_max) g_cycle_ms_max = realCycleMs;

    if (g_cycle_cnt >= STAT_WINDOW) {
        g_cycle_cnt = 0;
        g_cycle_ms_accum = 0;
        g_cycle_ms_max = 0;
    }
}