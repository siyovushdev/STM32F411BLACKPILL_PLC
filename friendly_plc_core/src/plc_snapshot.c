#include "friendly_plc/plc_snapshot.h"

#include "friendly_plc/plc.h"
#include "friendly_plc/plc_runtime.h"
#include "friendly_plc/plc_safety.h"

#include <string.h>

extern PlcGraph g_activeGraph;
extern bool g_activeGraphValid;
extern PlcRuntimeState g_plcRuntime;

bool plc_snapshot_get_runtime(PlcRuntimeSnapshot* out)
{
    if (out == NULL) {
        return false;
    }

    memset(out, 0, sizeof(*out));

    out->activeGraphValid = g_activeGraphValid;
    out->running = plc_is_running();
    out->safeOrFaulted = plc_is_safe_or_faulted();

    if (g_activeGraphValid) {
        out->cycleMs = g_activeGraph.cycleMs;
        out->nodeCount = g_activeGraph.nodeCount;
    }

    out->cycleCounter = g_plcRuntime.cycleCounter;
    out->lastCycleUs = g_plcRuntime.lastCycleUs;
    out->maxCycleUs = g_plcRuntime.maxCycleUs;
    out->faultCounter = g_plcRuntime.faultCounter;
    out->runtimeLastFault = g_plcRuntime.lastFault;

    return true;
}

uint16_t plc_snapshot_get_node_count(void)
{
    if (!g_activeGraphValid) {
        return 0u;
    }

    return g_activeGraph.nodeCount;
}

bool plc_snapshot_get_node(uint16_t index, PlcNodeSnapshot* out)
{
    if (out == NULL) {
        return false;
    }

    memset(out, 0, sizeof(*out));

    if (!g_activeGraphValid) {
        return false;
    }

    if (index >= g_activeGraph.nodeCount) {
        return false;
    }

    const PlcNode* n = &g_activeGraph.nodes[index];

    out->index = index;
    out->id = n->id;
    out->type = n->type;
    out->flags = n->flags;

    out->outB = n->out.b;
    out->outI = n->out.i;
    out->outF = n->out.f;

    out->forceEnabled = n->force_en;
    out->forceBool = n->force_bool;
    out->forceLeftMs = n->force_left_ms;

    out->tonActive = n->ton_active;
    out->tonAccumMs = n->ton_accum_ms;

    out->toffHolding = n->toff_holding;
    out->toffLeftMs = n->toff_left_ms;

    out->srQ = n->sr_q;
    out->trigPrev = n->trig_prev;

    out->acc = n->acc;
    out->prevClk = n->prev_clk;

    out->pidInited = n->pid_inited;
    out->pidI = n->pid_i;
    out->pidPrevMeas = n->pid_prev_meas;

    out->filterInited = n->filt_inited;
    out->filterPrev = n->filt_prev;

    out->rampInited = n->ramp_inited;
    out->rampPrev = n->ramp_prev;

    out->aoZeroHold = n->ao_zero_hold;

    return true;
}