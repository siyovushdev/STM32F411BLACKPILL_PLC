#include "plc_node_timer.h"
#include "friendly_plc/plc_config.h"
#include "friendly_plc/plc_log.h"

static inline bool get_bool(const PlcNode* n)
{
    if (!n) return false;

    switch (n->valueType) {
        case PLC_VAL_BOOL:  return n->out.b;
        case PLC_VAL_INT:   return n->out.i != 0;
        case PLC_VAL_FLOAT: return n->out.f != 0.0f;
        default:            return false;
    }
}

void plc_node_exec_ton(PlcNode* self, const PlcNode* a, uint32_t dt_ms)
{
    bool inSig = get_bool(a);

    if (inSig) {
        if (!self->ton_active) {
            self->ton_active = true;
            self->ton_accum_ms = 0;
#if PLC_LOG_ENABLED && PLC_LOG_GRAPH
            PLC_LOGT(PLC_LOG_TAG, "TON[%u] start T=%u", (unsigned)self->id, (unsigned)self->paramMs);
#endif
        } else {
            if (self->ton_accum_ms < self->paramMs) {
                self->ton_accum_ms += dt_ms;
                if (self->ton_accum_ms > self->paramMs) {
                    self->ton_accum_ms = self->paramMs;
                }
            }
        }
    } else {
        self->ton_active = false;
        self->ton_accum_ms = 0;
    }

    self->out.b = (self->ton_accum_ms >= self->paramMs);
}

void plc_node_exec_toff(PlcNode* self, const PlcNode* a, uint32_t dt_ms)
{
    bool inSig = get_bool(a);

    if (inSig) {
        self->out.b = true;
        self->toff_holding = true;
        self->toff_left_ms = self->paramMs;
    } else {
        if (self->toff_holding) {
            if (self->toff_left_ms <= dt_ms) {
                self->toff_left_ms = 0;
                self->toff_holding = false;
            } else {
                self->toff_left_ms -= dt_ms;
            }
        }

        self->out.b = self->toff_holding;
    }
}

void plc_node_exec_tp(PlcNode* self, const PlcNode* a, uint32_t dt_ms)
{
    bool a_now = get_bool(a);
    bool rising = a_now && !self->trig_prev;
    self->trig_prev = a_now;

    if (rising) {
        self->toff_holding = true;
        self->toff_left_ms = self->paramMs;
    }

    self->out.b = self->toff_holding;

    if (self->toff_holding) {
        if (dt_ms >= self->toff_left_ms) {
            self->toff_left_ms = 0;
            self->toff_holding = false;
        } else {
            self->toff_left_ms -= dt_ms;
        }
    }
}

void plc_node_exec_r_trig(PlcNode* self, const PlcNode* a)
{
    bool cur = get_bool(a);
    self->out.b = cur && !self->trig_prev;
    self->trig_prev = cur;
}

void plc_node_exec_f_trig(PlcNode* self, const PlcNode* a)
{
    bool cur = get_bool(a);
    self->out.b = !cur && self->trig_prev;
    self->trig_prev = cur;
}