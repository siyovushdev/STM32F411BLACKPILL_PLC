#include "plc_node_counter.h"


static inline bool get_bool(const PlcNode* n)
{
    if (!n) {
        return false;
    }

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

void plc_node_exec_ctu(
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b)
{
    bool clk = get_bool(a);
    bool rst = b ? get_bool(b) : false;

    if (rst) {
        self->acc = 0;
    }

    bool rising = clk && !self->prev_clk;
    self->prev_clk = clk;

    if (rising) {
        if (self->acc < INT32_MAX) {
            self->acc++;
        }
    }

    if (self->acc < 0) {
        self->acc = 0;
    }

    self->out.i = self->acc;
}

void plc_node_exec_ctd(
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b)
{
    bool clk = get_bool(a);
    bool rst = b ? get_bool(b) : false;

    if (rst) {
        self->acc = self->paramInt;

        if (self->acc < 0) {
            self->acc = 0;
        }
    }

    bool rising = clk && !self->prev_clk;
    self->prev_clk = clk;

    if (rising && self->acc > 0) {
        self->acc--;
    }

    if (self->acc < 0) {
        self->acc = 0;
    }

    self->out.i = self->acc;
}

void plc_node_exec_ctud(
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b)
{
    bool cu = get_bool(a);
    bool cd = get_bool(b);

    bool clk_any = cu || cd;
    bool rising = clk_any && !self->prev_clk;

    self->prev_clk = clk_any;

    if (rising) {

        if (cu && !cd) {

            if (self->acc < INT32_MAX) {
                self->acc++;
            }

        } else if (cd && !cu) {

            if (self->acc > 0) {
                self->acc--;
            }
        }
    }

    if (self->acc < 0) {
        self->acc = 0;
    }

    self->out.i = self->acc;
}