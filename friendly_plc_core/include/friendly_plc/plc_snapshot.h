#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "friendly_plc/plc.h"
#include "friendly_plc/plc_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    bool activeGraphValid;
    bool running;
    bool safeOrFaulted;

    uint32_t cycleMs;
    uint16_t nodeCount;

    uint32_t cycleCounter;
    uint32_t lastCycleUs;
    uint32_t maxCycleUs;
    uint32_t faultCounter;

    PlcRuntimeFault runtimeLastFault;

} PlcRuntimeSnapshot;

typedef struct
{
    uint16_t index;
    uint16_t id;

    PlcNodeType type;
    uint32_t flags;

    bool outB;
    int32_t outI;
    float outF;

    bool forceEnabled;
    bool forceBool;
    uint32_t forceLeftMs;

    bool tonActive;
    uint32_t tonAccumMs;

    bool toffHolding;
    uint32_t toffLeftMs;

    bool srQ;
    bool trigPrev;

    int32_t acc;
    bool prevClk;

    bool pidInited;
    float pidI;
    float pidPrevMeas;

    bool filterInited;
    float filterPrev;

    bool rampInited;
    float rampPrev;

    bool aoZeroHold;

} PlcNodeSnapshot;

bool plc_snapshot_get_runtime(PlcRuntimeSnapshot* out);
uint16_t plc_snapshot_get_node_count(void);
bool plc_snapshot_get_node(uint16_t index, PlcNodeSnapshot* out);

#ifdef __cplusplus
}
#endif