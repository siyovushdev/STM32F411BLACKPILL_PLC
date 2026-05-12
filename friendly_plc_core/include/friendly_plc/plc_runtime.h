#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "friendly_plc/plc_safety.h"

typedef enum
{
    PLC_RUNTIME_OK = 0,

    PLC_RUNTIME_FAULT_NULL_GRAPH,
    PLC_RUNTIME_FAULT_BAD_NODE_INDEX,
    PLC_RUNTIME_FAULT_BAD_NODE_TYPE,
    PLC_RUNTIME_FAULT_EXECUTION_TIMEOUT,
    PLC_RUNTIME_FAULT_STACK_CORRUPTION,
    PLC_RUNTIME_FAULT_INVALID_INPUT,
    PLC_RUNTIME_FAULT_INVALID_OUTPUT,
    PLC_RUNTIME_FAULT_DIV_ZERO,

} PlcRuntimeFault;

typedef struct
{
    bool safeMode;

    PlcRuntimeFault lastFault;

    uint32_t cycleCounter;

    uint32_t lastCycleUs;
    uint32_t maxCycleUs;

    uint32_t faultCounter;

} PlcRuntimeState;

PlcFaultCode plc_runtime_fault_to_fault_code(PlcRuntimeFault fault);

extern PlcRuntimeState g_plcRuntime;

void plc_runtime_init(void);

void plc_runtime_enter_fault(
        PlcRuntimeFault fault);

bool plc_runtime_is_faulted(void);

void plc_runtime_reset_fault(void);