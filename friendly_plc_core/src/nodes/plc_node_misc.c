#include "plc_node_misc.h"

static inline bool get_bool(const PlcNode* n)
{
    if (!n) return false;

    switch (n->valueType) {

        case PLC_VAL_BOOL:
            return n->out.b;

        case PLC_VAL_INT:
            return n->out.i != 0;

        case PLC_VAL_FLOAT:
            return n->out.f != 0.0f;

        default:
            return false;
    }
}

static inline float get_float(const PlcNode* n)
{
    if (!n) return 0.0f;

    switch (n->valueType) {

        case PLC_VAL_FLOAT:
            return n->out.f;

        case PLC_VAL_INT:
            return (float)n->out.i;

        case PLC_VAL_BOOL:
            return n->out.b ? 1.0f : 0.0f;

        default:
            return 0.0f;
    }
}

void plc_node_exec_hyst(
        PlcNode* self,
        const PlcNode* a)
{
    float x = get_float(a);

    float center = self->paramFloat;

    float h = (float)self->paramInt * 0.001f;

    if (h < 0.0f) {
        h = -h;
    }

    float hi = center + 0.5f * h;
    float lo = center - 0.5f * h;

    bool y = self->sr_q;

    if (x >= hi) {
        y = true;
    }

    else if (x <= lo) {
        y = false;
    }

    self->sr_q = y;
    self->out.b = y;
}

void plc_node_exec_heartbeat(
        PlcNode* self,
        uint32_t dt_ms)
{
    self->log_accum_ms += dt_ms;

    if (self->log_accum_ms >= self->paramMs) {
        self->log_accum_ms -= self->paramMs;
        self->out.b = !self->out.b;
    }
}

void plc_node_exec_alarm_latch(
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b)
{
    bool cond = get_bool(a);
    bool ack = b ? get_bool(b) : false;

    if (ack) {
        self->sr_q = false;
    }

    else if (cond) {
        self->sr_q = true;
    }

    self->out.b = self->sr_q;
}

void plc_node_exec_alarm_gen(
        PlcNode* self,
        const PlcNode* a)
{
    self->out.b = get_bool(a);
}

void plc_node_exec_window_check(
        PlcNode* self,
        const PlcNode* a)
{
    float x = get_float(a);

    float center = self->paramFloat;
    float width = (float)self->paramInt * 0.001f;

    float lo = center - width * 0.5f;
    float hi = center + width * 0.5f;

    self->out.b = (x >= lo && x <= hi);
}

void plc_node_exec_safe_output(
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b)
{
    bool allow = b ? get_bool(b) : true;

    switch (self->valueType) {

        case PLC_VAL_BOOL: {

            bool normal = get_bool(a);
            bool safe = (self->paramInt != 0);

            self->out.b = allow
                          ? normal
                          : safe;

            break;
        }

        case PLC_VAL_INT: {

            int32_t normal = a ? a->out.i : 0;
            int32_t safe = self->paramInt;

            self->out.i = allow
                          ? normal
                          : safe;

            break;
        }

        case PLC_VAL_FLOAT:
        default: {

            float normal = get_float(a);
            float safe = self->paramFloat;

            self->out.f = allow
                          ? normal
                          : safe;

            break;
        }
    }
}