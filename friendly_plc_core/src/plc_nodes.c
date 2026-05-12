#include "friendly_plc/plc.h"
#include "friendly_plc/plc_port.h"
#include "friendly_plc/plc_log.h"
#include "friendly_plc/plc_config.h"
#include "friendly_plc/plc_safety.h"
#include "friendly_plc/plc_runtime.h"

#include "plc_internal.h"

#include "nodes/plc_node_basic.h"
#include "nodes/plc_node_logic.h"
#include "nodes/plc_node_timer.h"
#include "nodes/plc_node_compare.h"
#include "nodes/plc_node_math.h"
#include "nodes/plc_node_counter.h"
#include "nodes/plc_node_memory.h"
#include "nodes/plc_node_filter.h"
#include "nodes/plc_node_pid.h"
#include "nodes/plc_node_io.h"
#include "nodes/plc_node_misc.h"
#include "nodes/plc_node_special_io.h"
#include "nodes/plc_node_exec.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// ---- локальные состояния для подавления лишних логов
static uint8_t s_di_init[PLC_MAX_NODES];
static uint8_t s_di_last[PLC_MAX_NODES];

static uint8_t s_ai_init[PLC_MAX_NODES];
static int32_t s_ai_last[PLC_MAX_NODES];
static uint32_t s_ai_last_ts[PLC_MAX_NODES];

static uint8_t s_do_init[PLC_MAX_NODES];
static uint8_t s_do_last[PLC_MAX_NODES];

static uint8_t s_ao_init[PLC_MAX_NODES];
static int32_t s_ao_last[PLC_MAX_NODES];

// Утилиты
static inline int32_t iabs32(int32_t v) { return v < 0 ? -v : v; }

typedef struct {
    bool stable;         // стабилизированное состояние, которое отдаём наружу
    bool candidate;      // кандидат в новое состояние (последний сырый)
    uint32_t sinceMs;        // с какого времени держится candidate
    uint32_t debounceMs;     // порог стабилизации
    bool inited;
} DiFilterState;

static DiFilterState di_filter[PLC_MAX_DI_CHANNELS];
static const uint32_t DI_DEBOUNCE_DEFAULT_MS = 30; // по умолчанию 30 мс

void plc_io_reset_runtime(void)
{
    memset(di_filter, 0, sizeof(di_filter));

    memset(s_di_init, 0, sizeof(s_di_init));
    memset(s_di_last, 0, sizeof(s_di_last));

    memset(s_ai_init, 0, sizeof(s_ai_init));
    memset(s_ai_last, 0, sizeof(s_ai_last));
    memset(s_ai_last_ts, 0, sizeof(s_ai_last_ts));

    memset(s_do_init, 0, sizeof(s_do_init));
    memset(s_do_last, 0, sizeof(s_do_last));

    memset(s_ao_init, 0, sizeof(s_ao_init));
    memset(s_ao_last, 0, sizeof(s_ao_last));
}

void plc_refresh_inputs_hw(uint32_t nowMs, PlcGraph *g) {
    for (uint16_t i = 0; i < g->nodeCount; i++) {
        PlcNode *n = &g->nodes[i];
        switch (n->type) {
            case PLC_NODE_DIGITAL_IN: {
                const uint32_t tnow = nowMs;

                int ch = n->paramInt;
                if (ch < 0 || ch >= (int)(sizeof(di_filter)/sizeof(di_filter[0]))) {
#if PLC_LOG_ENABLED
                    PLC_LOGW(PLC_LOG_TAG, "DI node %u invalid channel=%d", (unsigned)i, ch);
#endif
                    plc_enter_safe(PLC_FAULT_DOMAIN_IO, PLC_FAULT_IO_CHANNEL_INVALID, ch);
                    n->out.b = false;
                    break;
                }

                DiFilterState *fs = &di_filter[ch];
                bool raw = plc_port_read_di((uint16_t)ch);

                if (!fs->inited) {
                    fs->inited = true;
                    fs->stable = raw;
                    fs->candidate = raw;
                    fs->sinceMs = tnow;
                    if (fs->debounceMs == 0) fs->debounceMs = DI_DEBOUNCE_DEFAULT_MS;
#if PLC_LOG_ENABLED && PLC_LOG_INPUTS && PLC_LOG_DI
                    PLC_LOGT(PLC_LOG_TAG, "DI[%u] init ch=%d stable=%u db=%u",
                         (unsigned) i, (int) n->paramInt, (unsigned) fs->stable, (unsigned) fs->debounceMs);
#endif
                }

                if (raw != fs->candidate) {
                    fs->candidate = raw;
                    fs->sinceMs = tnow;
                } else {
                    if ((tnow - fs->sinceMs) >= fs->debounceMs && fs->stable != fs->candidate) {
                        fs->stable = fs->candidate;
#if PLC_LOG_ENABLED && PLC_LOG_INPUTS && PLC_LOG_DI
                        if (!s_di_init[i] || s_di_last[i] != (uint8_t) fs->stable) {
                            s_di_init[i] = 1;
                            s_di_last[i] = (uint8_t) fs->stable;
                            PLC_LOGT(PLC_LOG_TAG, "DI[%u] ch=%d STABLE->%u", (unsigned) i, (int) n->paramInt,
                                 (unsigned) fs->stable);
                        }
#endif
                    }
                }

                n->out.b = fs->stable;
                break;
            }
            case PLC_NODE_AI_IN: {
                int32_t mv = plc_port_read_ai_mv((uint16_t)n->paramInt);   // 0..10000 mV
                float volts = (float)mv * 0.001f;              // инженерное значение в В

                n->out.i = mv;      // integer representation: mV
                n->out.f = volts;   // float representation: V

#if PLC_LOG_ENABLED && PLC_LOG_INPUTS && PLC_LOG_AI
                uint8_t do_log = 0;
    uint32_t tnow = nowMs;
    if (!s_ai_init[i]) {
        s_ai_init[i] = 1;
        s_ai_last[i] = mv;
        s_ai_last_ts[i] = tnow;
        do_log = 1;
    } else {
        int32_t d = iabs32(mv - s_ai_last[i]);
        if ((tnow - s_ai_last_ts[i]) >= PLC_LOG_AI_PERIOD_MS || d >= PLC_LOG_AI_DELTA) do_log = 1;
    }
    if (do_log) {
        PLC_LOGT(PLC_LOG_TAG, "AI[%u] ch=%d mv=%ld volts=%.3f d=%ld",
             (unsigned)i,
             (int)n->paramInt,
             (long)mv,
             (double)volts,
             (long)(mv - s_ai_last[i]));
        s_ai_last[i] = mv;
        s_ai_last_ts[i] = tnow;
    }
#endif
                break;
            }
            default:
                break;
        }
    }
}

void plc_graph_step(PlcGraph *g, uint32_t dt_ms)
{
    for (uint16_t i = 0; i < g->nodeCount; i++) {

        PlcNode *self = &g->nodes[i];

        if ((self->inA >= (int16_t)g->nodeCount) ||
            (self->inB >= (int16_t)g->nodeCount)) {
            plc_runtime_enter_fault(PLC_RUNTIME_FAULT_BAD_NODE_INDEX);
            return;
        }

        const PlcNode *a =
                (self->inA >= 0)
                ? &g->nodes[self->inA]
                : NULL;

        const PlcNode *b =
                (self->inB >= 0)
                ? &g->nodes[self->inB]
                : NULL;

        PlcNodeExecFn exec =
                plc_node_get_exec_fn(self->type);

        if (exec) {
            exec(g, self, a, b, dt_ms);
        } else {
            plc_runtime_enter_fault(PLC_RUNTIME_FAULT_BAD_NODE_TYPE);
            return;
        }

        if (plc_is_safe_or_faulted()) {
            return;
        }
    }
}






