#include "plc_node_filter.h"

static inline float get_float(const PlcNode* n)
{
    if (!n) return 0.0f;

    switch (n->valueType) {
        case PLC_VAL_FLOAT: return n->out.f;
        case PLC_VAL_INT:   return (float)n->out.i;
        case PLC_VAL_BOOL:  return n->out.b ? 1.0f : 0.0f;
        default:            return 0.0f;
    }
}

static void exec_filter_avg_common(PlcNode* self, const PlcNode* a)
{
    float x = get_float(a);
    float alpha = (float)self->paramInt * 0.001f;

    if (!self->filt_inited) {
        self->filt_inited = true;
        self->filt_prev = x;
        self->out.f = x;
        return;
    }

    if (alpha <= 0.0f) {
        self->out.f = self->filt_prev;
        return;
    }

    if (alpha >= 1.0f) {
        self->filt_prev = x;
        self->out.f = x;
        return;
    }

    float y = self->filt_prev + alpha * (x - self->filt_prev);
    self->filt_prev = y;
    self->out.f = y;
}

void plc_node_exec_analog_avg(PlcNode* self, const PlcNode* a)
{
    exec_filter_avg_common(self, a);
}

void plc_node_exec_filter_avg(PlcNode* self, const PlcNode* a)
{
    exec_filter_avg_common(self, a);
}

void plc_node_exec_ramp(PlcNode* self, const PlcNode* a, uint32_t dt_ms)
{
    float x = get_float(a);
    float rate = (float)self->paramInt * 0.001f;
    float ts = (dt_ms > 0u) ? ((float)dt_ms * 0.001f) : 0.001f;
    float max_delta = rate * ts;

    if (!self->ramp_inited) {
        self->ramp_inited = true;
        self->ramp_prev = x;
        self->out.f = x;
        return;
    }

    float delta = x - self->ramp_prev;

    if (delta > max_delta) {
        delta = max_delta;
    } else if (delta < -max_delta) {
        delta = -max_delta;
    }

    float y = self->ramp_prev + delta;
    self->ramp_prev = y;
    self->out.f = y;
}