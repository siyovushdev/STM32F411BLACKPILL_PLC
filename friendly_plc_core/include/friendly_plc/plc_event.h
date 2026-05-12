#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PLC_EVENT_CAPACITY
#define PLC_EVENT_CAPACITY 128
#endif

typedef enum {
    PLC_EVENT_NONE = 0,
    PLC_EVENT_VALIDATE_ERROR,
    PLC_EVENT_UPLOAD_OK,
    PLC_EVENT_UPLOAD_FAIL,
    PLC_EVENT_ACTIVATE_OK,
    PLC_EVENT_ACTIVATE_FAIL,
    PLC_EVENT_FORCE_ON,
    PLC_EVENT_FORCE_OFF,
    PLC_EVENT_LOG_VALUE,
    PLC_EVENT_RUNTIME_ERROR
} PlcEventCode;

typedef struct {
    uint32_t ts_ms;
    uint16_t code;
    int16_t  a;
    int16_t  b;
} PlcEvent;

void plc_event_clear(void);
bool plc_event_push(PlcEventCode code, int16_t a, int16_t b);
bool plc_event_pop(PlcEvent* out);
uint16_t plc_event_count(void);

#ifdef __cplusplus
}
#endif