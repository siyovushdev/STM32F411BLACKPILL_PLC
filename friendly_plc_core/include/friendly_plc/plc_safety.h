#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLC_STATE_BOOT = 0,
    PLC_STATE_STOP,
    PLC_STATE_RUN,
    PLC_STATE_SAFE,
    PLC_STATE_FAULT
} PlcState;

typedef enum {
    PLC_FAULT_SEVERITY_INFO = 0,
    PLC_FAULT_SEVERITY_WARNING,
    PLC_FAULT_SEVERITY_SAFE,
    PLC_FAULT_SEVERITY_FAULT
} PlcFaultSeverity;

typedef enum {
    PLC_FAULT_DOMAIN_NONE = 0,
    PLC_FAULT_DOMAIN_RUNTIME,
    PLC_FAULT_DOMAIN_GRAPH,
    PLC_FAULT_DOMAIN_VALIDATION,
    PLC_FAULT_DOMAIN_PERSIST,
    PLC_FAULT_DOMAIN_PROTOCOL,
    PLC_FAULT_DOMAIN_WATCHDOG,
    PLC_FAULT_DOMAIN_IO
} PlcFaultDomain;

typedef enum {
    PLC_FAULT_NONE = 0,

    PLC_FAULT_NULL_GRAPH,
    PLC_FAULT_BAD_NODE_INDEX,
    PLC_FAULT_BAD_NODE_TYPE,
    PLC_FAULT_EXECUTION_TIMEOUT,
    PLC_FAULT_STACK_CORRUPTION,
    PLC_FAULT_INVALID_INPUT,
    PLC_FAULT_INVALID_OUTPUT,
    PLC_FAULT_DIV_ZERO,

    PLC_FAULT_INVALID_GRAPH,
    PLC_FAULT_PERSIST_CORRUPT,
    PLC_FAULT_PROTOCOL_CRC,
    PLC_FAULT_PROTOCOL_FRAME,
    PLC_FAULT_WATCHDOG_MISSED,
    PLC_FAULT_IO_CHANNEL_INVALID,
    PLC_FAULT_SCAN_OVERRUN,

    PLC_FAULT_USER_SAFE_REQUEST,
    PLC_FAULT_USER_STOP_REQUEST
} PlcFaultCode;

typedef struct {
    uint32_t ts_ms;
    uint32_t sequence;
    PlcFaultDomain domain;
    PlcFaultCode code;
    PlcFaultSeverity severity;
    int32_t detail;
    uint32_t counter_snapshot;
} PlcFaultRecord;

#ifndef PLC_FAULT_HISTORY_CAPACITY
#define PLC_FAULT_HISTORY_CAPACITY 16u
#endif

typedef struct {
    PlcState state;
    PlcState previous_state;

    bool safe_outputs_applied;
    bool fault_latched;

    PlcFaultCode last_fault;
    PlcFaultDomain last_domain;
    PlcFaultSeverity last_severity;
    int32_t last_detail;
    uint32_t fault_counter;
    uint32_t fault_sequence;

    uint16_t protocol_error_streak;
    uint16_t scan_overrun_streak;
    uint16_t watchdog_miss_streak;

    PlcFaultRecord history[PLC_FAULT_HISTORY_CAPACITY];
    uint16_t history_head;
    uint16_t history_count;
} PlcSafetyState;

extern PlcSafetyState g_plcSafety;

void plc_safety_init(void);
PlcState plc_get_state(void);
bool plc_is_running(void);
bool plc_is_safe_or_faulted(void);

void plc_request_stop(void);
bool plc_request_run(void);
void plc_enter_safe(PlcFaultDomain domain, PlcFaultCode code, int32_t detail);
void plc_enter_fault(PlcFaultDomain domain, PlcFaultCode code, int32_t detail);
bool plc_ack_faults(void);

void plc_fault_report(PlcFaultDomain domain,
                      PlcFaultCode code,
                      PlcFaultSeverity severity,
                      int32_t detail);

void plc_fault_note_protocol_error(PlcFaultCode code, int32_t detail);
void plc_fault_note_protocol_ok(void);
void plc_fault_note_persist_corrupt(int32_t detail);
void plc_fault_note_watchdog_miss(int32_t detail);
void plc_fault_note_scan_overrun(uint32_t elapsed_ms, uint32_t budget_ms);
void plc_fault_note_scan_ok(void);

void plc_safety_apply_safe_outputs_once(void);
void plc_safety_apply_safe_outputs_always(void);

bool plc_fault_history_get(uint16_t index_from_newest, PlcFaultRecord* out);
uint16_t plc_fault_history_count(void);

const char* plc_state_to_string(PlcState state);
const char* plc_fault_code_to_string(PlcFaultCode code);
const char* plc_fault_domain_to_string(PlcFaultDomain domain);

#ifdef __cplusplus
}
#endif
