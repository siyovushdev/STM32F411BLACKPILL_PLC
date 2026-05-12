#include "plc_app.h"

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "plc_diag.h"
#include "plc_graph_loader.h"
#include "plc_link.h"
#include "plc_link_uart.h"
#include "plc_persist.h"
#include "plc_persist_service.h"
#include "plc_platform.h"

#include "friendly_plc/plc_runtime.h"
#include "friendly_plc/plc_safety.h"

#include <stdbool.h>

extern UART_HandleTypeDef huart2;

#ifndef PLC_APP_SCAN_PERIOD_MS
#define PLC_APP_SCAN_PERIOD_MS 10u
#endif

#ifndef PLC_APP_DIAG_PERIOD_MS
#define PLC_APP_DIAG_PERIOD_MS 1000u
#endif

#ifndef PLC_APP_STACK_SCAN_WORDS
#define PLC_APP_STACK_SCAN_WORDS 1024u
#endif

#ifndef PLC_APP_STACK_LINK_RX_WORDS
#define PLC_APP_STACK_LINK_RX_WORDS 1536u
#endif

#ifndef PLC_APP_STACK_LINK_TX_WORDS
#define PLC_APP_STACK_LINK_TX_WORDS 768u
#endif

#ifndef PLC_APP_STACK_DIAG_WORDS
#define PLC_APP_STACK_DIAG_WORDS 512u
#endif

#ifndef PLC_APP_STACK_PERSIST_WORDS
#define PLC_APP_STACK_PERSIST_WORDS 1024u
#endif

static TaskHandle_t s_plc_scan_task = NULL;
static TaskHandle_t s_link_rx_task = NULL;
static TaskHandle_t s_link_tx_task = NULL;
static TaskHandle_t s_diag_task = NULL;
static TaskHandle_t s_persist_task = NULL;

static volatile bool s_initialized = false;
static volatile bool s_tasks_created = false;

static void plc_scan_task(void* argument);
static void link_rx_task(void* argument);
static void link_tx_task(void* argument);
static void diag_task(void* argument);
static void persist_task(void* argument);

bool plc_app_init(void)
{
    if (s_initialized) {
        return true;
    }

    plc_runtime_init();
    plc_diag_init();
    plc_graph_loader_init();
    plc_persist_init();

    if (!plc_persist_service_init()) {
        return false;
    }

    plc_link_init();

    if (!plc_platform_init()) {
        return false;
    }

    PlcPersistResult load_result = plc_persist_load_active();

    if (load_result == PLC_PERSIST_OK) {
        (void)plc_request_run();
    } else if (load_result == PLC_PERSIST_ERR_EMPTY) {
        plc_request_stop();
    } else {
        plc_fault_note_persist_corrupt((int32_t)load_result);
    }

    PlcLinkUartConfig uart_cfg = {
            .huart = &huart2,
            .on_frame = plc_link_on_frame,
            .user = NULL
    };

    if (!plc_link_uart_init(&uart_cfg)) {
        return false;
    }

    s_initialized = true;
    return true;
}

bool plc_app_create_tasks(void)
{
    if (s_tasks_created) {
        return true;
    }

    if (!s_initialized && !plc_app_init()) {
        return false;
    }

    if (xTaskCreate(plc_scan_task, "plcScan", PLC_APP_STACK_SCAN_WORDS, NULL,
                    tskIDLE_PRIORITY + 4u, &s_plc_scan_task) != pdPASS) {
        return false;
    }

    if (xTaskCreate(link_rx_task, "linkRx", PLC_APP_STACK_LINK_RX_WORDS, NULL,
                    tskIDLE_PRIORITY + 3u, &s_link_rx_task) != pdPASS) {
        return false;
    }

    if (xTaskCreate(link_tx_task, "linkTx", PLC_APP_STACK_LINK_TX_WORDS, NULL,
                    tskIDLE_PRIORITY + 2u, &s_link_tx_task) != pdPASS) {
        return false;
    }

    if (xTaskCreate(persist_task, "persist", PLC_APP_STACK_PERSIST_WORDS, NULL,
                    tskIDLE_PRIORITY + 1u, &s_persist_task) != pdPASS) {
        return false;
    }

    if (xTaskCreate(diag_task, "diag", PLC_APP_STACK_DIAG_WORDS, NULL,
                    tskIDLE_PRIORITY + 1u, &s_diag_task) != pdPASS) {
        return false;
    }

    plc_diag_register_all_tasks(s_plc_scan_task, PLC_APP_STACK_SCAN_WORDS,
                                s_link_rx_task, PLC_APP_STACK_LINK_RX_WORDS,
                                s_link_tx_task, PLC_APP_STACK_LINK_TX_WORDS,
                                s_persist_task, PLC_APP_STACK_PERSIST_WORDS,
                                s_diag_task, PLC_APP_STACK_DIAG_WORDS);

    s_tasks_created = true;
    return true;
}

static void plc_scan_task(void* argument)
{
    (void)argument;

    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(PLC_APP_SCAN_PERIOD_MS);

    for (;;) {
        plc_platform_scan_cycle();
        plc_platform_feed_watchdog();
        vTaskDelayUntil(&last_wake, period);
    }
}

static void link_rx_task(void* argument)
{
    plc_link_uart_rx_task(argument);
}

static void link_tx_task(void* argument)
{
    plc_link_uart_tx_task(argument);
}

static void persist_task(void* argument)
{
    plc_persist_service_task(argument);
}

static void diag_task(void* argument)
{
    (void)argument;

    const TickType_t period = pdMS_TO_TICKS(PLC_APP_DIAG_PERIOD_MS);

    for (;;) {
        plc_platform_on_diag_tick();
        vTaskDelay(period);
    }
}
