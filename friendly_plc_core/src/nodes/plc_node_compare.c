#include "plc_node_compare.h"

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

void plc_node_exec_compare_lt(PlcNode* self, const PlcNode* a, const PlcNode* b)
{
    self->out.b = get_float(a) < get_float(b);
}

void plc_node_exec_compare_gt(PlcNode* self, const PlcNode* a, const PlcNode* b)
{
    self->out.b = get_float(a) > get_float(b);
}

void plc_node_exec_compare_ge(PlcNode* self, const PlcNode* a, const PlcNode* b)
{
    self->out.b = get_float(a) >= get_float(b);
}