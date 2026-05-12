#include "friendly_plc/plc.h"

static inline bool get_bool(const PlcNode *n)
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

void plc_node_exec_and2(
        PlcNode *self,
        const PlcNode *a,
        const PlcNode *b)
{
    self->out.b = get_bool(a) && get_bool(b);
}

void plc_node_exec_or2(
        PlcNode *self,
        const PlcNode *a,
        const PlcNode *b)
{
    self->out.b = get_bool(a) || get_bool(b);
}

void plc_node_exec_not(
        PlcNode *self,
        const PlcNode *a)
{
    self->out.b = !get_bool(a);
}

void plc_node_exec_sr(
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b)
{
    bool set_sig = get_bool(a);
    bool reset_sig = get_bool(b);

    if (reset_sig) {
        self->sr_q = false;
    } else if (set_sig) {
        self->sr_q = true;
    }

    self->out.b = self->sr_q;
}