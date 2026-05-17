#include "plc_diag.h"

#include "plc_platform.h"

#include <string.h>

typedef struct {
    PlcDiagTaskInfo tasks[PLC_DIAG_TASK_COUNT];
    uint32_t scan_avg_us;
    uint32_t scan_max_us;
    uint32_t work_avg_us;
    uint32_t work_max_us;
    uint32_t cycle_real_avg_us;
    uint32_t cycle_real_max_us;
    uint32_t scan_limit_ms;
    uint32_t cpu_load_x100;
    uint32_t scan_long_steps;
    bool perf_inited;
} PlcDiagContext;

static PlcDiagContext s_diag;

static uint32_t task_stack_free_words(TaskHandle_t handle)
{
    if (handle == NULL) {
        return 0u;
    }

#if (INCLUDE_uxTaskGetStackHighWaterMark == 1)
    return (uint32_t)uxTaskGetStackHighWaterMark(handle);
#else
    (void)handle;
    return PLC_DIAG_STACK_UNAVAILABLE;
#endif
}

static uint32_t task_stack_used_words(uint32_t configured_words, uint32_t free_words)
{
    if (free_words == PLC_DIAG_STACK_UNAVAILABLE) {
        return PLC_DIAG_STACK_UNAVAILABLE;
    }

    if (free_words > configured_words) {
        return 0u;
    }

    return configured_words - free_words;
}

static void fill_task_stack(const PlcDiagTaskInfo* info,
                            uint32_t* free_words,
                            uint32_t* used_words,
                            uint32_t* size_words)
{
    if (free_words == NULL || used_words == NULL || size_words == NULL || info == NULL) {
        return;
    }

    *size_words = info->configured_stack_words;
    *free_words = task_stack_free_words(info->handle);
    *used_words = task_stack_used_words(info->configured_stack_words, *free_words);
}

void plc_diag_init(void)
{
    memset(&s_diag, 0, sizeof(s_diag));
}

void plc_diag_register_task(PlcDiagTaskId id,
                            TaskHandle_t handle,
                            uint32_t configured_stack_words)
{
    if ((uint32_t)id >= (uint32_t)PLC_DIAG_TASK_COUNT) {
        return;
    }

    s_diag.tasks[id].handle = handle;
    s_diag.tasks[id].configured_stack_words = configured_stack_words;
}

void plc_diag_register_all_tasks(TaskHandle_t scan_task,
                                 uint32_t scan_stack_words,
                                 TaskHandle_t link_rx_task,
                                 uint32_t link_rx_stack_words,
                                 TaskHandle_t link_tx_task,
                                 uint32_t link_tx_stack_words,
                                 TaskHandle_t persist_task,
                                 uint32_t persist_stack_words,
                                 TaskHandle_t diag_task,
                                 uint32_t diag_stack_words)
{
    plc_diag_register_task(PLC_DIAG_TASK_SCAN, scan_task, scan_stack_words);
    plc_diag_register_task(PLC_DIAG_TASK_LINK_RX, link_rx_task, link_rx_stack_words);
    plc_diag_register_task(PLC_DIAG_TASK_LINK_TX, link_tx_task, link_tx_stack_words);
    plc_diag_register_task(PLC_DIAG_TASK_PERSIST, persist_task, persist_stack_words);
    plc_diag_register_task(PLC_DIAG_TASK_DIAG, diag_task, diag_stack_words);
}

static uint32_t avg_iir_u32(uint32_t old_value, uint32_t new_value)
{
    if (new_value >= old_value) {
        return old_value + ((new_value - old_value) / 8u);
    }

    return old_value - ((old_value - new_value) / 8u);
}

void plc_diag_note_scan_metrics(uint32_t scan_us,
                                uint32_t work_us,
                                uint32_t cycle_real_us,
                                uint32_t scan_limit_ms)
{
    if (!s_diag.perf_inited) {
        s_diag.scan_avg_us = scan_us;
        s_diag.work_avg_us = work_us;
        s_diag.cycle_real_avg_us = cycle_real_us;
        s_diag.perf_inited = true;
    } else {
        s_diag.scan_avg_us = avg_iir_u32(s_diag.scan_avg_us, scan_us);
        s_diag.work_avg_us = avg_iir_u32(s_diag.work_avg_us, work_us);
        s_diag.cycle_real_avg_us = avg_iir_u32(s_diag.cycle_real_avg_us, cycle_real_us);
    }

    if (scan_us > s_diag.scan_max_us) {
        s_diag.scan_max_us = scan_us;
    }

    if (work_us > s_diag.work_max_us) {
        s_diag.work_max_us = work_us;
    }

    if (cycle_real_us > s_diag.cycle_real_max_us) {
        s_diag.cycle_real_max_us = cycle_real_us;
    }

    s_diag.scan_limit_ms = scan_limit_ms;

    if (cycle_real_us > 0u) {
        s_diag.cpu_load_x100 = (work_us * 10000u) / cycle_real_us;
    }

    if (scan_limit_ms > 0u && scan_us > (scan_limit_ms * 1000u)) {
        s_diag.scan_long_steps++;
    }
}

void plc_diag_get_status(PlcDiagStatus* out_status)
{
    if (out_status == NULL) {
        return;
    }

    memset(out_status, 0, sizeof(*out_status));

    out_status->uptime_ms = plc_platform_now_ms();
    out_status->heap_total_bytes = (uint32_t)configTOTAL_HEAP_SIZE;
    out_status->heap_free_bytes = (uint32_t)xPortGetFreeHeapSize();
    out_status->heap_min_ever_free_bytes = (uint32_t)xPortGetMinimumEverFreeHeapSize();

    for (uint32_t i = 0u; i < (uint32_t)PLC_DIAG_TASK_COUNT; i++) {
        if (s_diag.tasks[i].handle != NULL) {
            out_status->task_registered_flags |= (1u << i);
        }
    }

    fill_task_stack(&s_diag.tasks[PLC_DIAG_TASK_SCAN],
                    &out_status->scan_stack_free_words,
                    &out_status->scan_stack_used_words,
                    &out_status->scan_stack_size_words);

    fill_task_stack(&s_diag.tasks[PLC_DIAG_TASK_LINK_RX],
                    &out_status->link_rx_stack_free_words,
                    &out_status->link_rx_stack_used_words,
                    &out_status->link_rx_stack_size_words);

    fill_task_stack(&s_diag.tasks[PLC_DIAG_TASK_LINK_TX],
                    &out_status->link_tx_stack_free_words,
                    &out_status->link_tx_stack_used_words,
                    &out_status->link_tx_stack_size_words);

    fill_task_stack(&s_diag.tasks[PLC_DIAG_TASK_PERSIST],
                    &out_status->persist_stack_free_words,
                    &out_status->persist_stack_used_words,
                    &out_status->persist_stack_size_words);

    fill_task_stack(&s_diag.tasks[PLC_DIAG_TASK_DIAG],
                    &out_status->diag_stack_free_words,
                    &out_status->diag_stack_used_words,
                    &out_status->diag_stack_size_words);

    out_status->scan_avg_us = s_diag.scan_avg_us;
    out_status->scan_max_us = s_diag.scan_max_us;
    out_status->work_avg_us = s_diag.work_avg_us;
    out_status->work_max_us = s_diag.work_max_us;
    out_status->cycle_real_avg_us = s_diag.cycle_real_avg_us;
    out_status->cycle_real_max_us = s_diag.cycle_real_max_us;
    out_status->scan_limit_ms = s_diag.scan_limit_ms;
    out_status->cpu_load_x100 = s_diag.cpu_load_x100;
    out_status->scan_long_steps = s_diag.scan_long_steps;
}
