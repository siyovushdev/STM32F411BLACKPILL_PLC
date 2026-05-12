#pragma once

#include "plc_persist.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool initialized;
    bool busy;
    bool pending;
    bool copying;
    uint32_t request_count;
    uint32_t success_count;
    uint32_t fail_count;
    uint32_t rejected_count;
    uint32_t last_version;
    uint32_t last_size;
    uint32_t last_crc32;
    uint32_t last_duration_ms;
    PlcPersistResult last_result;
} PlcPersistServiceStatus;

bool plc_persist_service_init(void);
bool plc_persist_service_request_save(const uint8_t* image, uint32_t size, uint32_t version);
void plc_persist_service_task(void* argument);
void plc_persist_service_get_status(PlcPersistServiceStatus* out_status);

#ifdef __cplusplus
}
#endif
