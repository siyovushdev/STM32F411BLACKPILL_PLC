#include "plc_persist_service.h"
#include "friendly_plc/plc_safety.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
typedef struct {
    uint32_t size;
    uint32_t version;
} PlcPersistSaveRequest;

typedef struct {
    QueueHandle_t queue;
    volatile bool initialized;
    volatile bool busy;
    volatile bool pending;
    volatile bool copying;
    uint32_t request_count;
    uint32_t success_count;
    uint32_t fail_count;
    uint32_t rejected_count;
    uint32_t last_version;
    uint32_t last_size;
    uint32_t last_crc32;
    uint32_t last_duration_ms;
    PlcPersistResult last_result;
} PlcPersistServiceContext;

static PlcPersistServiceContext s_service;
static uint8_t s_pending_image[PLC_PERSIST_MAX_IMAGE_SIZE];

static uint32_t crc32_update(uint32_t crc, const uint8_t* data, uint32_t len)
{
    crc = ~crc;
    for (uint32_t i = 0u; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0u; bit < 8u; bit++) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1u) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

static uint32_t crc32_calc(const uint8_t* data, uint32_t len)
{
    return crc32_update(0u, data, len);
}

bool plc_persist_service_init(void)
{
    if (s_service.initialized) {
        return true;
    }

    memset(&s_service, 0, sizeof(s_service));
    s_service.last_result = PLC_PERSIST_OK;

    s_service.queue = xQueueCreate(1u, sizeof(PlcPersistSaveRequest));
    if (s_service.queue == NULL) {
        return false;
    }

    s_service.initialized = true;
    return true;
}

bool plc_persist_service_request_save(const uint8_t* image, uint32_t size, uint32_t version)
{
    if (!s_service.initialized || s_service.queue == NULL || image == NULL) {
        return false;
    }
    if (size == 0u || size > PLC_PERSIST_MAX_IMAGE_SIZE) {
        return false;
    }

    taskENTER_CRITICAL();
    if (s_service.busy || s_service.pending || s_service.copying) {
        s_service.rejected_count++;
        taskEXIT_CRITICAL();
        return false;
    }
    s_service.copying = true;
    taskEXIT_CRITICAL();

    memcpy(s_pending_image, image, size);

    PlcPersistSaveRequest req;
    req.size = size;
    req.version = version;

    taskENTER_CRITICAL();
    s_service.copying = false;
    s_service.pending = true;
    s_service.request_count++;
    s_service.last_version = version;
    s_service.last_size = size;
    s_service.last_crc32 = crc32_calc(s_pending_image, size);
    taskEXIT_CRITICAL();

    if (xQueueSend(s_service.queue, &req, 0u) != pdPASS) {
        taskENTER_CRITICAL();
        s_service.pending = false;
        s_service.rejected_count++;
        taskEXIT_CRITICAL();
        return false;
    }

    return true;
}

void plc_persist_service_task(void* argument)
{
    (void)argument;

    PlcPersistSaveRequest req;

    for (;;) {
        if (xQueueReceive(s_service.queue, &req, portMAX_DELAY) != pdPASS) {
            continue;
        }

        taskENTER_CRITICAL();
        s_service.pending = false;
        s_service.busy = true;
        taskEXIT_CRITICAL();

        const TickType_t t0 = xTaskGetTickCount();
        PlcPersistResult r = plc_persist_save_image(s_pending_image, req.size, req.version);
        const TickType_t t1 = xTaskGetTickCount();

        taskENTER_CRITICAL();
        s_service.last_duration_ms = (uint32_t)((t1 - t0) * portTICK_PERIOD_MS);
        s_service.last_result = r;
        if (r == PLC_PERSIST_OK) {
            s_service.success_count++;
        } else {
            s_service.fail_count++;

            plc_fault_report(
                    PLC_FAULT_DOMAIN_PERSIST,
                    PLC_FAULT_PERSIST_CORRUPT,
                    PLC_FAULT_SEVERITY_WARNING,
                    (int32_t)r
            );
        }
        s_service.busy = false;
        taskEXIT_CRITICAL();
    }
}

void plc_persist_service_get_status(PlcPersistServiceStatus* out_status)
{
    if (out_status == NULL) {
        return;
    }

    taskENTER_CRITICAL();
    out_status->initialized = s_service.initialized;
    out_status->busy = s_service.busy;
    out_status->pending = s_service.pending;
    out_status->copying = s_service.copying;
    out_status->request_count = s_service.request_count;
    out_status->success_count = s_service.success_count;
    out_status->fail_count = s_service.fail_count;
    out_status->rejected_count = s_service.rejected_count;
    out_status->last_version = s_service.last_version;
    out_status->last_size = s_service.last_size;
    out_status->last_crc32 = s_service.last_crc32;
    out_status->last_duration_ms = s_service.last_duration_ms;
    out_status->last_result = s_service.last_result;
    taskEXIT_CRITICAL();
}
