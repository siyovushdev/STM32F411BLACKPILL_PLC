#include "plc_node_io.h"

#include "friendly_plc/plc_port.h"
#include "friendly_plc/plc_config.h"

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

void plc_node_exec_do(
        PlcNode* self,
        const PlcNode* a)
{
    bool desired = get_bool(a);

    if (self->force_en &&
        (self->flags & PLC_NODE_FLAG_FORCE)) {

        desired = self->force_bool;
    }

    plc_port_write_do(
            (uint16_t)self->paramInt,
            desired);

    self->out.b = desired;
}

void plc_node_exec_ao(
        PlcNode* self,
        const PlcNode* a)
{
    float v = get_float(a);

    if (v < 0.0f) {
        v = 0.0f;
    }

    if (v > 100.0f) {
        v = 100.0f;
    }

    if (self->force_en &&
        (self->flags & PLC_NODE_FLAG_FORCE)) {

        v = self->force_bool ? 100.0f : 0.0f;
        self->ao_zero_hold = false;
    }

    else {

        if (!self->ao_zero_hold) {

            if (v <= (float)PLC_AO_DEADBAND_OFF_PCT) {
                self->ao_zero_hold = true;
            }

        } else {

            if (v >= (float)PLC_AO_DEADBAND_ON_PCT) {
                self->ao_zero_hold = false;
            }
        }

        if (self->ao_zero_hold) {
            v = 0.0f;
        }
    }

    plc_port_write_ao_percent(
            (uint16_t)self->paramInt,
            v);

    self->out.f = v;
}