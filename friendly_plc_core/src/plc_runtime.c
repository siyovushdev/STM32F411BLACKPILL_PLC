#include "friendly_plc/plc.h"
#include "friendly_plc/plc_port.h"
#include "friendly_plc/plc_log.h"
#include "friendly_plc/plc_event.h"
#include "friendly_plc/plc_config.h"
#include "friendly_plc/plc_error.h"
#include "plc_internal.h"
#include "friendly_plc/plc_runtime.h"
#include "friendly_plc/plc_safety.h"

#include <stdint.h>
#include <stdbool.h>

static uint32_t s_lastNowMs = 0;
static uint32_t s_tick_last_hb = 0;

PlcGraph g_activeGraph;
PlcGraph g_stagingGraph;

bool g_activeGraphValid = false;
bool g_stagingGraphValid = false;
volatile bool g_needSwapGraph = false;

PlcRuntimeState g_plcRuntime;

static void plc_node_reset_runtime(PlcNode *n) {
    n->out.b = false;
    n->out.i = 0;
    n->out.f = 0.0f;

    n->sr_q = false;
    n->trig_prev = false;

    n->ton_active = false;
    n->ton_accum_ms = 0;

    n->toff_holding = false;
    n->toff_left_ms = 0;

    n->force_en = false;
    n->force_bool = false;
    n->force_left_ms = 0;

    // --- PID runtime ---
    n->pid_i = 0.0f;
    n->pid_prev_meas = 0.0f;
    n->pid_inited = false;

    // --- Filter / Ramp runtime ---
    n->filt_prev = 0.0f;
    n->filt_inited = false;

    n->ramp_prev = 0.0f;
    n->ramp_inited = false;

    n->log_accum_ms = 0;

    // CTU/CTD/CTUD
    n->acc = 0;
    n->prev_clk = false;

    n->ao_zero_hold = false;
}

void plc_graph_reset_runtime(PlcGraph *g) {
    for (uint16_t i = 0; i < g->nodeCount; i++) {
        plc_node_reset_runtime(&g->nodes[i]);
    }
}

bool plc_upload_graph(const PlcGraph *src) {
#if PLC_LOG_ENABLED && PLC_LOG_UPLOAD
    PLC_LOGT(PLC_LOG_TAG, "upload: requested (nodeCount=%u, cycleMs=%u)", (unsigned) src->nodeCount,
         (unsigned) src->cycleMs);
#endif

    g_stagingGraph = *src;

    PlcError err = plc_validate_graph_ex(&g_stagingGraph);

    if (err != PLC_OK) {
#if PLC_LOG_ENABLED && PLC_LOG_UPLOAD
        PLC_LOGT(PLC_LOG_TAG,
                 "upload: INVALID graph, err=%d (%s)",
                 (int)err,
                 plc_error_to_string(err));
#endif

        g_stagingGraphValid = false;
        plc_event_push(PLC_EVENT_VALIDATE_ERROR, (int16_t)err, 0);
        plc_enter_safe(PLC_FAULT_DOMAIN_VALIDATION, PLC_FAULT_INVALID_GRAPH, (int32_t)err);
        return false;
    }

#if PLC_LOG_ENABLED && PLC_LOG_UPLOAD
    PLC_LOGT(PLC_LOG_TAG, "upload: graph valid (nodes=%u, cycleMs=%u)", (unsigned) g_stagingGraph.nodeCount,
         (unsigned) g_stagingGraph.cycleMs);
#endif

    plc_graph_reset_runtime(&g_stagingGraph);
#if PLC_LOG_ENABLED && PLC_LOG_UPLOAD
    PLC_LOGT(PLC_LOG_TAG, "upload: runtime reset done");
#endif

    g_stagingGraphValid = true;
//  g_needSwapGraph = true; // оставлено как в твоём коде (закомментировано)
    return true;
}

bool plc_request_activate_graph(void) {
    if (!g_stagingGraphValid) {
        plc_event_push(PLC_EVENT_ACTIVATE_FAIL, 1, 0); // 1=staging invalid
#if PLC_LOG_ENABLED && PLC_LOG_ACTIVATE
        PLC_LOGT(PLC_LOG_TAG, "activate: staging invalid -> reject");
#endif
        return false;
    }
    g_needSwapGraph = true; // TaskPlcScan обработает
#if PLC_LOG_ENABLED && PLC_LOG_ACTIVATE
    PLC_LOGT(PLC_LOG_TAG, "activate: requested (will swap on next tick)");
#endif

//    plc_event_push(PLC_EVENT_ACTIVATE_OK, (int16_t)g_stagingGraph.nodeCount, (int16_t)g_stagingGraph.cycleMs);
    return true;
}



bool plc_release_output(uint16_t nodeIndex)
{
#if PLC_LOG_ENABLED && PLC_LOG_CMDS
    PLC_LOGT(PLC_LOG_TAG, "release_output: req node=%u", (unsigned)nodeIndex);
#endif

    if (!g_activeGraphValid) {
#if PLC_LOG_ENABLED && PLC_LOG_CMDS
        PLC_LOGT(PLC_LOG_TAG, "release_output: no active graph");
#endif
        return false;
    }

    if (nodeIndex >= g_activeGraph.nodeCount) {
#if PLC_LOG_ENABLED && PLC_LOG_CMDS
        PLC_LOGT(PLC_LOG_TAG, "release_output: bad nodeIndex=%u (nodes=%u)",
             (unsigned)nodeIndex, (unsigned)g_activeGraph.nodeCount);
#endif
        return false;
    }

    PlcNode *n = &g_activeGraph.nodes[nodeIndex];

    // Смысла нет снимать форс с не-выходных узлов, но и не ошибка
    if (n->type != PLC_NODE_DIGITAL_OUT &&
        n->type != PLC_NODE_AO) {
#if PLC_LOG_ENABLED && PLC_LOG_CMDS
        PLC_LOGT(PLC_LOG_TAG, "release_output: node[%u] type=%u not DO/AO",
             (unsigned)nodeIndex, (unsigned)n->type);
#endif
        // можно вернуть true, чтобы на верхнем уровне не считалось ошибкой
        return true;
    }


    n->force_en      = false;
    n->flags        &= ~PLC_NODE_FLAG_FORCE;
    n->force_left_ms = 0;
    // force_bool можно не трогать, он больше не используется


#if PLC_LOG_ENABLED && PLC_LOG_CMDS
    PLC_LOGT(PLC_LOG_TAG, "release_output: node[%u] OK", (unsigned)nodeIndex);
#endif

    return true;
}


bool plc_force_output(uint16_t nodeIndex, bool value, uint32_t holdMs)
{
#if PLC_LOG_ENABLED && PLC_LOG_CMDS
    PLC_LOGT(PLC_LOG_TAG, "force_output: req node=%u val=%u holdMs=%u",
         (unsigned)nodeIndex, (unsigned)value, (unsigned)holdMs);
#endif


    do {
        if (!g_activeGraphValid) {
#if PLC_LOG_ENABLED && PLC_LOG_CMDS
            PLC_LOGT(PLC_LOG_TAG, "force_output: no active graph");
#endif
            break;
        }

        if (nodeIndex >= g_activeGraph.nodeCount) {
#if PLC_LOG_ENABLED && PLC_LOG_CMDS
            PLC_LOGT(PLC_LOG_TAG, "force_output: bad nodeIndex=%u (nodeCount=%u)",
                 (unsigned)nodeIndex, (unsigned)g_activeGraph.nodeCount);
#endif
            break;
        }

        PlcNode *n = &g_activeGraph.nodes[nodeIndex];

        if (n->type != PLC_NODE_DIGITAL_OUT &&
            n->type != PLC_NODE_AO) {
#if PLC_LOG_ENABLED && PLC_LOG_CMDS
            PLC_LOGT(PLC_LOG_TAG, "force_output: node[%u] type=%u not DO/AO",
                 (unsigned)nodeIndex, (unsigned)n->type);
#endif
            break;
        }

        n->force_en      = true;
        n->force_bool    = value;
        n->force_left_ms = holdMs;
        n->flags        |= PLC_NODE_FLAG_FORCE;

#if PLC_LOG_ENABLED && PLC_LOG_CMDS
        PLC_LOGT(PLC_LOG_TAG, "force_output: node[%u] en=1 val=%u left=%u",
             (unsigned)nodeIndex, (unsigned)n->force_bool, (unsigned)n->force_left_ms);
#endif


        return true;
    } while (0);


    return false;
}

static void plc_apply_pending_cmds(PlcGraph* g, uint32_t dt_ms);

void plc_tick(uint32_t nowMs)
{
    if (plc_is_safe_or_faulted()) {
        plc_safety_apply_safe_outputs_once();
        plc_port_feed_watchdog();
        return;
    }

    uint32_t periodMs = 10;
    if (g_activeGraphValid && g_activeGraph.cycleMs > 0) {
        periodMs = g_activeGraph.cycleMs;
    }

    uint32_t dt;
    if (s_lastNowMs == 0) {
        dt = periodMs;                 // первый цикл
    } else {
        dt = nowMs - s_lastNowMs;      // реальное время
    }
    s_lastNowMs = nowMs;

    // clamp, чтобы не “взорвать” PID/таймеры после паузы
    // (например, после дебага/стопа/высоких приоритетов)
    const uint32_t DT_MAX = periodMs * 4u;   // типично 2..10 периодов
    if (dt > DT_MAX) dt = DT_MAX;
    if (dt == 0) dt = 1;                     // защита от 0

    // дальше все таймеры/пиды используют dt (а не periodMs)
    // plc_refresh_inputs_hw(nowMs, dt);
    // plc_eval_graph(dt);

    // 1) активация нового графа
    if (g_needSwapGraph && g_stagingGraphValid) {


        g_activeGraph = g_stagingGraph;
        g_activeGraphValid = true;
        g_needSwapGraph = false;
        plc_graph_reset_runtime(&g_activeGraph);
        plc_io_reset_runtime();

        plc_event_push(PLC_EVENT_ACTIVATE_OK,
                       (int16_t)g_activeGraph.nodeCount,
                       (int16_t)g_activeGraph.cycleMs);

        plc_mem_reset_all();

        // СБРОС ВСЕХ ВЫХОДОВ
        plc_safety_apply_safe_outputs_always();
        plc_ack_faults();
        plc_request_run();



#if PLC_LOG_ENABLED && PLC_LOG_ACTIVATE
        PLC_LOGT(PLC_LOG_TAG, "activate: swapped (nodes=%u, cycleMs=%u)",
             (unsigned) g_activeGraph.nodeCount, (unsigned) g_activeGraph.cycleMs);
#endif
    }

    // 2) нет активного графа
    if (!g_activeGraphValid) {
#if PLC_LOG_ENABLED && PLC_LOG_TICK
        uint32_t tnow = nowMs;
        if (tnow - s_tick_last_hb >= PLC_TICK_HEARTBEAT_MS) {
            s_tick_last_hb = tnow;
            PLC_LOGT(PLC_LOG_TAG, "tick: idle (no active graph)");
        }
#endif
        plc_port_feed_watchdog();
        return;
    }

    // 3) dt мониторинг
#if PLC_LOG_ENABLED && PLC_LOG_TICK
    if (dt > PLC_TICK_DT_WARN_MS) {
        PLC_LOGT(PLC_LOG_TAG, "tick: dt=%u ms (WARN)", (unsigned) dt);
    } else {
        uint32_t tnow = nowMs;
        if (tnow - s_tick_last_hb >= PLC_TICK_HEARTBEAT_MS) {
            s_tick_last_hb = tnow;
            PLC_LOGT(PLC_LOG_TAG, "tick: ok dt=%u ms nodes=%u cycle=%u",
                 (unsigned) dt, (unsigned) g_activeGraph.nodeCount, (unsigned) g_activeGraph.cycleMs);
        }
    }
#endif

    // AI update is handled by platform layer if needed

    if (!plc_is_running()) {
        plc_safety_apply_safe_outputs_once();
        plc_port_feed_watchdog();
        return;
    }

    // 4) цикл ПЛК
    uint32_t execStartMs = plc_port_now_ms();

    plc_refresh_inputs_hw(nowMs, &g_activeGraph);
    plc_apply_pending_cmds(&g_activeGraph, dt);
    plc_graph_step(&g_activeGraph, dt);

    uint32_t execElapsedMs = plc_port_now_ms() - execStartMs;
    uint32_t budgetMs = (periodMs * PLC_SCAN_OVERRUN_BUDGET_PERCENT) / 100u;
    if (budgetMs == 0u) {
        budgetMs = 1u;
    }

    g_plcRuntime.cycleCounter++;
    g_plcRuntime.lastCycleUs = execElapsedMs * 1000u;
    if (g_plcRuntime.lastCycleUs > g_plcRuntime.maxCycleUs) {
        g_plcRuntime.maxCycleUs = g_plcRuntime.lastCycleUs;
    }

    if (execElapsedMs > budgetMs || dt > (periodMs * 2u)) {
        plc_fault_note_scan_overrun(execElapsedMs > budgetMs ? execElapsedMs : dt, budgetMs);
    } else {
        plc_fault_note_scan_ok();
    }

    // 5) watchdog
    plc_port_feed_watchdog();
}

static void plc_apply_pending_cmds(PlcGraph *g, uint32_t dt_ms)
{
    for (uint16_t i = 0; i < g->nodeCount; i++) {
        PlcNode *n = &g->nodes[i];

        if (n->force_en && (n->flags & PLC_NODE_FLAG_FORCE)) {

            // 0 = бесконечный форс, таймер не считаем вообще
            if (n->force_left_ms == 0) {
                continue;
            }

            if (n->force_left_ms <= dt_ms) {
                n->force_en      = false;
                n->flags        &= ~PLC_NODE_FLAG_FORCE;
                n->force_left_ms = 0;
#if PLC_LOG_ENABLED && PLC_LOG_CMDS
                PLC_LOGT(PLC_LOG_TAG, "force: node[%u] expired", (unsigned)i);
#endif
            } else {
                n->force_left_ms -= dt_ms;
            }
        }
    }
}

void plc_runtime_init(void)
{
    plc_safety_init();

    g_plcRuntime.safeMode = false;

    g_plcRuntime.lastFault = PLC_RUNTIME_OK;

    g_plcRuntime.cycleCounter = 0;

    g_plcRuntime.lastCycleUs = 0;
    g_plcRuntime.maxCycleUs = 0;

    g_plcRuntime.faultCounter = 0;
}

static PlcFaultCode plc_runtime_fault_to_safety_fault(PlcRuntimeFault fault)
{
    switch (fault) {
        case PLC_RUNTIME_OK: return PLC_FAULT_NONE;
        case PLC_RUNTIME_FAULT_NULL_GRAPH: return PLC_FAULT_NULL_GRAPH;
        case PLC_RUNTIME_FAULT_BAD_NODE_INDEX: return PLC_FAULT_BAD_NODE_INDEX;
        case PLC_RUNTIME_FAULT_BAD_NODE_TYPE: return PLC_FAULT_BAD_NODE_TYPE;
        case PLC_RUNTIME_FAULT_EXECUTION_TIMEOUT: return PLC_FAULT_EXECUTION_TIMEOUT;
        case PLC_RUNTIME_FAULT_STACK_CORRUPTION: return PLC_FAULT_STACK_CORRUPTION;
        case PLC_RUNTIME_FAULT_INVALID_INPUT: return PLC_FAULT_INVALID_INPUT;
        case PLC_RUNTIME_FAULT_INVALID_OUTPUT: return PLC_FAULT_INVALID_OUTPUT;
        case PLC_RUNTIME_FAULT_DIV_ZERO: return PLC_FAULT_DIV_ZERO;
        default: return PLC_FAULT_BAD_NODE_TYPE;
    }
}

void plc_runtime_enter_fault(
        PlcRuntimeFault fault)
{
    if (fault == PLC_RUNTIME_OK) {
        return;
    }

    g_plcRuntime.safeMode = true;
    g_plcRuntime.lastFault = fault;
    g_plcRuntime.faultCounter++;

    plc_enter_fault(PLC_FAULT_DOMAIN_RUNTIME,
                    plc_runtime_fault_to_safety_fault(fault),
                    (int32_t)fault);
}

bool plc_runtime_is_faulted(void)
{
    return g_plcRuntime.safeMode;
}

void plc_runtime_reset_fault(void)
{
    g_plcRuntime.safeMode = false;
    g_plcRuntime.lastFault = PLC_RUNTIME_OK;

    (void)plc_ack_faults();
    if (g_activeGraphValid) {
        (void)plc_request_run();
    }
}
