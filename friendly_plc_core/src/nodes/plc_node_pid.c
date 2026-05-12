#include "plc_node_pid.h"

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

void plc_node_exec_pid(
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b,
        uint32_t dt_ms)
{
    float sp = get_float(a);
    float pv = get_float(b);

    float kp = self->paramFloat;
    float ki = (float)self->paramInt * 0.001f;
    float kd = (float)self->paramMs * 0.001f;

    float ts = (dt_ms > 0u)
               ? ((float)dt_ms * 0.001f)
               : 0.001f;

    float e = sp - pv;

    if (!self->pid_inited) {
        self->pid_i = 0.0f;
        self->pid_prev_meas = pv;
        self->pid_inited = true;
    }

    float i_next = self->pid_i + ki * e * ts;

    float d_meas = (pv - self->pid_prev_meas) / ts;
    float d_term = -kd * d_meas;

    self->pid_prev_meas = pv;

    float p_term = kp * e;

    float u = p_term + i_next + d_term;

    if (self->flags & PLC_NODE_FLAG_PID_CLAMP_0_100) {

        if (u > 100.0f) {
            u = 100.0f;
            if (e > 0.0f) {
                i_next = self->pid_i;
            }
        }

        else if (u < 0.0f) {
            u = 0.0f;
            if (e < 0.0f) {
                i_next = self->pid_i;
            }
        }
    }

    else if (self->flags & PLC_NODE_FLAG_PID_CLAMP_0_1) {

        if (u > 1.0f) {
            u = 1.0f;
            if (e > 0.0f) {
                i_next = self->pid_i;
            }
        }

        else if (u < 0.0f) {
            u = 0.0f;
            if (e < 0.0f) {
                i_next = self->pid_i;
            }
        }
    }

    self->pid_i = i_next;

    switch (self->valueType) {

        case PLC_VAL_BOOL:
            self->out.b = (u != 0.0f);
            break;

        case PLC_VAL_INT:
            self->out.i = (int32_t)u;
            break;

        case PLC_VAL_FLOAT:
        default:
            self->out.f = u;
            break;
    }
}