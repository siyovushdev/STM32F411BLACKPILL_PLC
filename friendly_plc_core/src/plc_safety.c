#include "friendly_plc/plc_safety.h"
#include "friendly_plc/plc_port.h"
#include "friendly_plc/plc_event.h"
#include "friendly_plc/plc_config.h"
#include "friendly_plc/plc_log.h"

#include <string.h>

PlcSafetyState g_plcSafety;

static void plc_safety_set_state(PlcState new_state)
{
    if (g_plcSafety.state == new_state) {
        return;
    }

    g_plcSafety.previous_state = g_plcSafety.state;
    g_plcSafety.state = new_state;

    if (new_state == PLC_STATE_RUN) {
        g_plcSafety.safe_outputs_applied = false;
    }

#if PLC_LOG_ENABLED
    PLC_LOGT(PLC_LOG_TAG, "state: %s -> %s",
             plc_state_to_string(g_plcSafety.previous_state),
             plc_state_to_string(g_plcSafety.state));
#endif
}

static void plc_fault_push_history(PlcFaultDomain domain,
                                   PlcFaultCode code,
                                   PlcFaultSeverity severity,
                                   int32_t detail)
{
    PlcFaultRecord* r = &g_plcSafety.history[g_plcSafety.history_head];

    g_plcSafety.fault_sequence++;

    r->ts_ms = plc_port_now_ms();
    r->sequence = g_plcSafety.fault_sequence;
    r->domain = domain;
    r->code = code;
    r->severity = severity;
    r->detail = detail;
    r->counter_snapshot = g_plcSafety.fault_counter;

    g_plcSafety.history_head = (uint16_t)((g_plcSafety.history_head + 1u) % PLC_FAULT_HISTORY_CAPACITY);
    if (g_plcSafety.history_count < PLC_FAULT_HISTORY_CAPACITY) {
        g_plcSafety.history_count++;
    }
}

void plc_safety_init(void)
{
    memset(&g_plcSafety, 0, sizeof(g_plcSafety));
    g_plcSafety.state = PLC_STATE_BOOT;
    g_plcSafety.previous_state = PLC_STATE_BOOT;
    plc_safety_set_state(PLC_STATE_STOP);
}

PlcState plc_get_state(void)
{
    return g_plcSafety.state;
}

bool plc_is_running(void)
{
    return g_plcSafety.state == PLC_STATE_RUN;
}

bool plc_is_safe_or_faulted(void)
{
    return g_plcSafety.state == PLC_STATE_SAFE || g_plcSafety.state == PLC_STATE_FAULT;
}

void plc_safety_apply_safe_outputs_always(void)
{
    plc_port_stop_pwm();
    plc_port_set_safe_outputs();
    plc_port_reset_outputs();
}

void plc_safety_apply_safe_outputs_once(void)
{
    if (g_plcSafety.safe_outputs_applied) {
        return;
    }

    plc_safety_apply_safe_outputs_always();
    g_plcSafety.safe_outputs_applied = true;
}

void plc_fault_report(PlcFaultDomain domain,
                      PlcFaultCode code,
                      PlcFaultSeverity severity,
                      int32_t detail)
{
    if (code == PLC_FAULT_NONE || severity == PLC_FAULT_SEVERITY_INFO) {
        return;
    }

    g_plcSafety.fault_counter++;
    g_plcSafety.last_domain = domain;
    g_plcSafety.last_fault = code;
    g_plcSafety.last_severity = severity;
    g_plcSafety.last_detail = detail;

    plc_fault_push_history(domain, code, severity, detail);
    plc_event_push(PLC_EVENT_RUNTIME_ERROR, (int16_t)code, (int16_t)detail);

#if PLC_LOG_ENABLED
    PLC_LOGW(PLC_LOG_TAG, "fault: domain=%s code=%s severity=%d detail=%ld",
             plc_fault_domain_to_string(domain),
             plc_fault_code_to_string(code),
             (int)severity,
             (long)detail);
#endif

    if (severity == PLC_FAULT_SEVERITY_SAFE) {
        plc_safety_set_state(PLC_STATE_SAFE);
        plc_safety_apply_safe_outputs_once();
        return;
    }

    if (severity == PLC_FAULT_SEVERITY_FAULT) {
        g_plcSafety.fault_latched = true;
        plc_safety_set_state(PLC_STATE_FAULT);
        plc_safety_apply_safe_outputs_once();
    }
}

void plc_enter_safe(PlcFaultDomain domain, PlcFaultCode code, int32_t detail)
{
    plc_fault_report(domain, code, PLC_FAULT_SEVERITY_SAFE, detail);
}

void plc_enter_fault(PlcFaultDomain domain, PlcFaultCode code, int32_t detail)
{
    plc_fault_report(domain, code, PLC_FAULT_SEVERITY_FAULT, detail);
}

void plc_request_stop(void)
{
    plc_fault_report(PLC_FAULT_DOMAIN_RUNTIME,
                     PLC_FAULT_USER_STOP_REQUEST,
                     PLC_FAULT_SEVERITY_WARNING,
                     0);
    plc_safety_set_state(PLC_STATE_STOP);
    plc_safety_apply_safe_outputs_once();
}

bool plc_request_run(void)
{
    if (g_plcSafety.fault_latched || g_plcSafety.state == PLC_STATE_FAULT) {
        return false;
    }

    if (g_plcSafety.state == PLC_STATE_SAFE) {
        return false;
    }

    plc_safety_set_state(PLC_STATE_RUN);
    return true;
}

bool plc_ack_faults(void)
{
    if (g_plcSafety.state == PLC_STATE_RUN) {
        return false;
    }

    g_plcSafety.fault_latched = false;
    g_plcSafety.last_fault = PLC_FAULT_NONE;
    g_plcSafety.last_domain = PLC_FAULT_DOMAIN_NONE;
    g_plcSafety.last_severity = PLC_FAULT_SEVERITY_INFO;
    g_plcSafety.last_detail = 0;
    g_plcSafety.protocol_error_streak = 0;
    g_plcSafety.scan_overrun_streak = 0;
    g_plcSafety.watchdog_miss_streak = 0;
    g_plcSafety.safe_outputs_applied = false;

    plc_safety_set_state(PLC_STATE_STOP);
    plc_safety_apply_safe_outputs_once();
    return true;
}

void plc_fault_note_protocol_error(PlcFaultCode code, int32_t detail)
{
    g_plcSafety.protocol_error_streak++;

    if (g_plcSafety.protocol_error_streak >= PLC_PROTOCOL_ERROR_FAULT_THRESHOLD) {
        plc_enter_fault(PLC_FAULT_DOMAIN_PROTOCOL, code, detail);
    } else if (g_plcSafety.protocol_error_streak >= PLC_PROTOCOL_ERROR_SAFE_THRESHOLD) {
        plc_enter_safe(PLC_FAULT_DOMAIN_PROTOCOL, code, detail);
    }
}

void plc_fault_note_protocol_ok(void)
{
    g_plcSafety.protocol_error_streak = 0;
}

void plc_fault_note_persist_corrupt(int32_t detail)
{
    plc_enter_fault(PLC_FAULT_DOMAIN_PERSIST, PLC_FAULT_PERSIST_CORRUPT, detail);
}

void plc_fault_note_watchdog_miss(int32_t detail)
{
    g_plcSafety.watchdog_miss_streak++;
    if (g_plcSafety.watchdog_miss_streak >= PLC_WATCHDOG_MISS_FAULT_THRESHOLD) {
        plc_enter_fault(PLC_FAULT_DOMAIN_WATCHDOG, PLC_FAULT_WATCHDOG_MISSED, detail);
    } else {
        plc_enter_safe(PLC_FAULT_DOMAIN_WATCHDOG, PLC_FAULT_WATCHDOG_MISSED, detail);
    }
}

void plc_fault_note_scan_overrun(uint32_t elapsed_ms, uint32_t budget_ms)
{
    g_plcSafety.scan_overrun_streak++;

    int32_t detail = (int32_t)((elapsed_ms << 16u) | (budget_ms & 0xFFFFu));

    if (g_plcSafety.scan_overrun_streak >= PLC_SCAN_OVERRUN_FAULT_THRESHOLD) {
        plc_enter_fault(PLC_FAULT_DOMAIN_RUNTIME, PLC_FAULT_SCAN_OVERRUN, detail);
    } else if (g_plcSafety.scan_overrun_streak >= PLC_SCAN_OVERRUN_SAFE_THRESHOLD) {
        plc_enter_safe(PLC_FAULT_DOMAIN_RUNTIME, PLC_FAULT_SCAN_OVERRUN, detail);
    }
}

void plc_fault_note_scan_ok(void)
{
    g_plcSafety.scan_overrun_streak = 0;
}

bool plc_fault_history_get(uint16_t index_from_newest, PlcFaultRecord* out)
{
    if (!out || index_from_newest >= g_plcSafety.history_count) {
        return false;
    }

    uint16_t newest = (g_plcSafety.history_head == 0u)
            ? (uint16_t)(PLC_FAULT_HISTORY_CAPACITY - 1u)
            : (uint16_t)(g_plcSafety.history_head - 1u);

    uint16_t idx = (uint16_t)((newest + PLC_FAULT_HISTORY_CAPACITY - index_from_newest) % PLC_FAULT_HISTORY_CAPACITY);
    *out = g_plcSafety.history[idx];
    return true;
}

uint16_t plc_fault_history_count(void)
{
    return g_plcSafety.history_count;
}

const char* plc_state_to_string(PlcState state)
{
    switch (state) {
        case PLC_STATE_BOOT: return "BOOT";
        case PLC_STATE_STOP: return "STOP";
        case PLC_STATE_RUN: return "RUN";
        case PLC_STATE_SAFE: return "SAFE";
        case PLC_STATE_FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

const char* plc_fault_domain_to_string(PlcFaultDomain domain)
{
    switch (domain) {
        case PLC_FAULT_DOMAIN_NONE: return "NONE";
        case PLC_FAULT_DOMAIN_RUNTIME: return "RUNTIME";
        case PLC_FAULT_DOMAIN_GRAPH: return "GRAPH";
        case PLC_FAULT_DOMAIN_VALIDATION: return "VALIDATION";
        case PLC_FAULT_DOMAIN_PERSIST: return "PERSIST";
        case PLC_FAULT_DOMAIN_PROTOCOL: return "PROTOCOL";
        case PLC_FAULT_DOMAIN_WATCHDOG: return "WATCHDOG";
        case PLC_FAULT_DOMAIN_IO: return "IO";
        default: return "UNKNOWN";
    }
}

const char* plc_fault_code_to_string(PlcFaultCode code)
{
    switch (code) {
        case PLC_FAULT_NONE: return "NONE";
        case PLC_FAULT_NULL_GRAPH: return "NULL_GRAPH";
        case PLC_FAULT_BAD_NODE_INDEX: return "BAD_NODE_INDEX";
        case PLC_FAULT_BAD_NODE_TYPE: return "BAD_NODE_TYPE";
        case PLC_FAULT_EXECUTION_TIMEOUT: return "EXECUTION_TIMEOUT";
        case PLC_FAULT_STACK_CORRUPTION: return "STACK_CORRUPTION";
        case PLC_FAULT_INVALID_INPUT: return "INVALID_INPUT";
        case PLC_FAULT_INVALID_OUTPUT: return "INVALID_OUTPUT";
        case PLC_FAULT_DIV_ZERO: return "DIV_ZERO";
        case PLC_FAULT_INVALID_GRAPH: return "INVALID_GRAPH";
        case PLC_FAULT_PERSIST_CORRUPT: return "PERSIST_CORRUPT";
        case PLC_FAULT_PROTOCOL_CRC: return "PROTOCOL_CRC";
        case PLC_FAULT_PROTOCOL_FRAME: return "PROTOCOL_FRAME";
        case PLC_FAULT_WATCHDOG_MISSED: return "WATCHDOG_MISSED";
        case PLC_FAULT_IO_CHANNEL_INVALID: return "IO_CHANNEL_INVALID";
        case PLC_FAULT_SCAN_OVERRUN: return "SCAN_OVERRUN";
        case PLC_FAULT_USER_SAFE_REQUEST: return "USER_SAFE_REQUEST";
        case PLC_FAULT_USER_STOP_REQUEST: return "USER_STOP_REQUEST";
        default: return "UNKNOWN";
    }
}
