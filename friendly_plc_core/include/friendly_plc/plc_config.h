#pragma once

// =========================================================
// PLC LIMITS
// =========================================================

#define PLC_MAX_NODES 100

#define PLC_CYCLE_MS_MIN 5
#define PLC_CYCLE_MS_MAX 100

#define PLC_MEM_BOOL_COUNT 32
#define PLC_MEM_INT_COUNT  32
#define PLC_MEM_REAL_COUNT 32

#ifndef PLC_MAX_DI_CHANNELS
#define PLC_MAX_DI_CHANNELS 64
#endif

// =========================================================
// ANALOG OUTPUT
// =========================================================

#define PLC_AO_DEADBAND_OFF_PCT 2
#define PLC_AO_DEADBAND_ON_PCT  3
#define PLC_AO_LOG_DELTA_PCT    1

// =========================================================
// MEMORY
// =========================================================

#define PLC_RESET_MEM_ON_ACTIVATE 0

// =========================================================
// LOGGING
// =========================================================

#ifndef PLC_LOG_ENABLED
#define PLC_LOG_ENABLED 1
#endif

#ifndef PLC_LOG_TAG
#define PLC_LOG_TAG "PLC"
#endif

// =========================================================
// VALIDATION
// =========================================================

#ifndef PLC_LOG_VALIDATE
#define PLC_LOG_VALIDATE 1
#endif

// =========================================================
// GRAPH / UPLOAD
// =========================================================

#ifndef PLC_LOG_UPLOAD
#define PLC_LOG_UPLOAD 1
#endif

#ifndef PLC_LOG_ACTIVATE
#define PLC_LOG_ACTIVATE 1
#endif

#ifndef PLC_LOG_GRAPH
#define PLC_LOG_GRAPH 1
#endif

// =========================================================
// PLC TICK
// =========================================================

#ifndef PLC_LOG_TICK
#define PLC_LOG_TICK 0
#endif

#ifndef PLC_TICK_HEARTBEAT_MS
#define PLC_TICK_HEARTBEAT_MS 1000u
#endif

#ifndef PLC_TICK_DT_WARN_MS
#define PLC_TICK_DT_WARN_MS (PLC_TICK_HEARTBEAT_MS * 2u)
#endif

// =========================================================
// INPUT LOGGING
// =========================================================

#ifndef PLC_LOG_INPUTS
#define PLC_LOG_INPUTS 1
#endif

#ifndef PLC_LOG_DI
#define PLC_LOG_DI 0
#endif

#ifndef PLC_LOG_AI
#define PLC_LOG_AI 0
#endif

#ifndef PLC_LOG_AI_DELTA
#define PLC_LOG_AI_DELTA 50
#endif

#ifndef PLC_LOG_AI_PERIOD_MS
#define PLC_LOG_AI_PERIOD_MS 1000u
#endif

// =========================================================
// COMMANDS
// =========================================================

#ifndef PLC_LOG_CMDS
#define PLC_LOG_CMDS 1
#endif
// =========================================================
// SAFETY / FAULT ESCALATION
// =========================================================

#ifndef PLC_SCAN_OVERRUN_SAFE_THRESHOLD
#define PLC_SCAN_OVERRUN_SAFE_THRESHOLD 3u
#endif

#ifndef PLC_SCAN_OVERRUN_FAULT_THRESHOLD
#define PLC_SCAN_OVERRUN_FAULT_THRESHOLD 10u
#endif

#ifndef PLC_SCAN_OVERRUN_BUDGET_PERCENT
#define PLC_SCAN_OVERRUN_BUDGET_PERCENT 100u
#endif

#ifndef PLC_PROTOCOL_ERROR_SAFE_THRESHOLD
#define PLC_PROTOCOL_ERROR_SAFE_THRESHOLD 5u
#endif

#ifndef PLC_PROTOCOL_ERROR_FAULT_THRESHOLD
#define PLC_PROTOCOL_ERROR_FAULT_THRESHOLD 20u
#endif

#ifndef PLC_WATCHDOG_MISS_FAULT_THRESHOLD
#define PLC_WATCHDOG_MISS_FAULT_THRESHOLD 2u
#endif
