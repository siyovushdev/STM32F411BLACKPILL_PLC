#include "plc_node_memory.h"

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

void plc_node_exec_mem_bool(PlcNode* self, const PlcNode* a, const PlcNode* b)
{
    uint16_t idx = (uint16_t)self->paramInt;

    if (idx >= PLC_MEM_BOOL_COUNT) {
        self->out.b = false;
        return;
    }

    bool value = plc_mem_get_bool(idx);

    if (self->inA >= 0 && a) {
        bool we = true;

        if (self->inB >= 0 && b) {
            we = get_bool(b);
        }

        if (we) {
            value = get_bool(a);
            plc_mem_set_bool(idx, value);
        }
    }

    self->out.b = value;
}

void plc_node_exec_mem_int(PlcNode* self, const PlcNode* a, const PlcNode* b)
{
    uint16_t idx = (uint16_t)self->paramInt;

    if (idx >= PLC_MEM_INT_COUNT) {
        self->out.i = 0;
        return;
    }

    int32_t value = plc_mem_get_int(idx);

    if (self->inA >= 0 && a) {
        bool we = true;

        if (self->inB >= 0 && b) {
            we = get_bool(b);
        }

        if (we) {
            value = get_int(a);
            plc_mem_set_int(idx, value);
        }
    }

    self->out.i = value;
}

void plc_node_exec_mem_real(PlcNode* self, const PlcNode* a, const PlcNode* b)
{
    uint16_t idx = (uint16_t)self->paramInt;

    if (idx >= PLC_MEM_REAL_COUNT) {
        self->out.f = 0.0f;
        return;
    }

    float value = plc_mem_get_real(idx);

    if (self->inA >= 0 && a) {
        bool we = true;

        if (self->inB >= 0 && b) {
            we = get_bool(b);
        }

        if (we) {
            value = get_float(a);
            plc_mem_set_real(idx, value);
        }
    }

    self->out.f = value;
}