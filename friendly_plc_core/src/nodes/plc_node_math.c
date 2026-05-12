#include "plc_node_math.h"

#include <stdint.h>

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

static inline int32_t get_int(const PlcNode* n)
{
    if (!n) return 0;

    switch (n->valueType) {
        case PLC_VAL_INT:   return n->out.i;
        case PLC_VAL_FLOAT: return (int32_t)(n->out.f + (n->out.f >= 0 ? 0.5f : -0.5f));
        case PLC_VAL_BOOL:  return n->out.b ? 1 : 0;
        default:            return 0;
    }
}

void plc_node_exec_scale(PlcNode* self, const PlcNode* a)
{
    float x = get_float(a);
    float k = self->paramFloat;
    float b = (float)self->paramInt * 0.001f;

    self->out.f = k * x + b;
}

void plc_node_exec_add(PlcNode* self, const PlcNode* a, const PlcNode* b)
{
    self->out.f = get_float(a) + get_float(b);
}

void plc_node_exec_limit(PlcNode* self, const PlcNode* a)
{
    float x = get_float(a);
    float mn = (float)self->paramInt * 0.001f;
    float mx = (float)self->paramMs * 0.001f;

    if (mn > mx) {
        float t = mn;
        mn = mx;
        mx = t;
    }

    if (x < mn) {
        self->out.f = mn;
    } else if (x > mx) {
        self->out.f = mx;
    } else {
        self->out.f = x;
    }
}

void plc_node_exec_math_op(PlcNode* self, const PlcNode* a, const PlcNode* b)
{
    int op = self->paramInt;

    if (self->valueType == PLC_VAL_INT) {
        int32_t va = get_int(a);
        int32_t vb = get_int(b);
        int32_t r = 0;

        switch (op) {
            case 0: r = va + vb; break;
            case 1: r = va - vb; break;
            case 2: r = va * vb; break;
            case 3:
                r = (vb == 0) ? self->out.i : (va / vb);
                break;
            default:
                r = 0;
                break;
        }

        self->out.i = r;
        return;
    }

    float va = get_float(a);
    float vb = get_float(b);
    float r = 0.0f;

    switch (op) {
        case 0: r = va + vb; break;
        case 1: r = va - vb; break;
        case 2: r = va * vb; break;
        case 3:
            r = (vb == 0.0f) ? self->out.f : (va / vb);
            break;
        default:
            r = 0.0f;
            break;
    }

    self->out.f = r;
}