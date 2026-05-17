#pragma once

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLC_DIAG_STACK_UNAVAILABLE 0xFFFFFFFFu

typedef enum {
    PLC_DIAG_TASK_SCAN    = 0,
    PLC_DIAG_TASK_LINK_RX = 1,
    PLC_DIAG_TASK_LINK_TX = 2,
    PLC_DIAG_TASK_PERSIST = 3,
    PLC_DIAG_TASK_DIAG    = 4,
    PLC_DIAG_TASK_COUNT   = 5
} PlcDiagTaskId;

typedef struct {
    TaskHandle_t handle;
    uint32_t configured_stack_words;
} PlcDiagTaskInfo;

typedef struct {
    uint32_t uptime_ms;

    uint32_t heap_total_bytes;
    uint32_t heap_free_bytes;
    uint32_t heap_min_ever_free_bytes;

    uint32_t task_registered_flags;

    uint32_t scan_stack_free_words;
    uint32_t scan_stack_used_words;
    uint32_t scan_stack_size_words;

    uint32_t link_rx_stack_free_words;
    uint32_t link_rx_stack_used_words;
    uint32_t link_rx_stack_size_words;

    uint32_t link_tx_stack_free_words;
    uint32_t link_tx_stack_used_words;
    uint32_t link_tx_stack_size_words;

    uint32_t persist_stack_free_words;
    uint32_t persist_stack_used_words;
    uint32_t persist_stack_size_words;

    uint32_t diag_stack_free_words;
    uint32_t diag_stack_used_words;
    uint32_t diag_stack_size_words;

    uint32_t scan_avg_us;
    uint32_t scan_max_us;
    uint32_t work_avg_us;
    uint32_t work_max_us;
    uint32_t cycle_real_avg_us;
    uint32_t cycle_real_max_us;
    uint32_t scan_limit_ms;
    uint32_t cpu_load_x100;
    uint32_t scan_long_steps;
} PlcDiagStatus;

void plc_diag_init(void);

void plc_diag_register_task(PlcDiagTaskId id,
                            TaskHandle_t handle,
                            uint32_t configured_stack_words);

void plc_diag_register_all_tasks(TaskHandle_t scan_task,
                                 uint32_t scan_stack_words,
                                 TaskHandle_t link_rx_task,
                                 uint32_t link_rx_stack_words,
                                 TaskHandle_t link_tx_task,
                                 uint32_t link_tx_stack_words,
                                 TaskHandle_t persist_task,
                                 uint32_t persist_stack_words,
                                 TaskHandle_t diag_task,
                                 uint32_t diag_stack_words);

void plc_diag_get_status(PlcDiagStatus* out_status);

void plc_diag_note_scan_metrics(uint32_t scan_us,
                                uint32_t work_us,
                                uint32_t cycle_real_us,
                                uint32_t scan_limit_ms);

#ifdef __cplusplus
}
#endif
