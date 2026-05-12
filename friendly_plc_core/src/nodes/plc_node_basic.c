#include <stddef.h>
#include "plc_node_basic.h"

#include "friendly_plc/plc_event.h"

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

static inline int32_t get_int(const PlcNode* n)
{
    if (!n) return 0;

    switch (n->valueType) {
        case PLC_VAL_INT:   return n->out.i;
        case PLC_VAL_FLOAT: return (int32_t)(n->out.f + (n->out.f >= 0.0f ? 0.5f : -0.5f));
        case PLC_VAL_BOOL:  return n->out.b ? 1 : 0;
        default:            return 0;
    }
}

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

void plc_node_exec_const_bool(PlcNode* self)
{
    self->out.b = (self->paramInt != 0);
}

void plc_node_exec_const_int(PlcNode* self)
{
    switch (self->valueType) {
        case PLC_VAL_BOOL:
            self->out.b = (self->paramInt != 0);
            break;

        case PLC_VAL_INT:
            self->out.i = self->paramInt;
            break;

        case PLC_VAL_FLOAT:
        default: {
            float v = self->paramFloat;

            if (v == 0.0f && self->paramInt != 0) {
                v = (float)self->paramInt * 0.001f;
            }

            self->out.f = v;
            break;
        }
    }
}

void plc_node_exec_const_float(PlcNode* self)
{
    self->out.f = self->paramFloat;
}

void plc_node_exec_mux2(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b)
{
    const PlcNode* s = NULL;

    if (self->paramInt >= 0 && self->paramInt < (int32_t)g->nodeCount) {
        s = &g->nodes[self->paramInt];
    }

    bool sel = get_bool(s);

    switch (self->valueType) {
        case PLC_VAL_BOOL:
            self->out.b = sel ? get_bool(a) : get_bool(b);
            break;

        case PLC_VAL_INT:
            self->out.i = sel ? get_int(a) : get_int(b);
            break;

        case PLC_VAL_FLOAT:
        default:
            self->out.f = sel ? get_float(a) : get_float(b);
            break;
    }
}

void plc_node_exec_log(PlcNode* self, const PlcNode* a, uint32_t dt_ms)
{
    float x = get_float(a);
    self->out.f = x;

    self->log_accum_ms += dt_ms;

    if (self->log_accum_ms < self->paramMs) {
        return;
    }

    self->log_accum_ms = 0;

    int16_t v;

    if (x > 327.67f) {
        v = 32767;
    } else if (x < -327.68f) {
        v = -32768;
    } else {
        v = (int16_t)(x * 100.0f);
    }

    plc_event_push(PLC_EVENT_LOG_VALUE, (int16_t)self->id, v);
}