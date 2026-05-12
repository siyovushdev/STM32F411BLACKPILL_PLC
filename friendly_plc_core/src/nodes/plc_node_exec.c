#include "plc_node_exec.h"

#include "plc_node_basic.h"
#include "plc_node_logic.h"
#include "plc_node_timer.h"
#include "plc_node_compare.h"
#include "plc_node_counter.h"
#include "plc_node_memory.h"
#include "plc_node_filter.h"
#include "plc_node_math.h"
#include "plc_node_pid.h"
#include "plc_node_io.h"
#include "plc_node_misc.h"
#include "plc_node_special_io.h"

#define UNUSED(x) (void)(x)

/* ================= BASIC ================= */

static void exec_const_bool(
        PlcGraph* g,
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b,
        uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(a);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_const_bool(self);
}

static void exec_const_int(
        PlcGraph* g,
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b,
        uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(a);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_const_int(self);
}

static void exec_const_float(
        PlcGraph* g,
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b,
        uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(a);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_const_float(self);
}

static void exec_mux2(
        PlcGraph* g,
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b,
        uint32_t dt_ms)
{
    UNUSED(dt_ms);

    plc_node_exec_mux2(g, self, a, b);
}

static void exec_log(
        PlcGraph* g,
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b,
        uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);

    plc_node_exec_log(self, a, dt_ms);
}

/* ================= LOGIC ================= */

static void exec_and2(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_and2(self, a, b);
}

static void exec_or2(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_or2(self, a, b);
}

static void exec_not(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_not(self, a);
}

static void exec_sr(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_sr(self, a, b);
}

/* ================= TIMERS ================= */

static void exec_ton(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);

    plc_node_exec_ton(self, a, dt_ms);
}

static void exec_toff(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);

    plc_node_exec_toff(self, a, dt_ms);
}

static void exec_tp(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);

    plc_node_exec_tp(self, a, dt_ms);
}

static void exec_r_trig(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_r_trig(self, a);
}

static void exec_f_trig(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_f_trig(self, a);
}

/* ================= COMPARE ================= */

static void exec_compare_lt(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_compare_lt(self, a, b);
}

static void exec_compare_gt(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_compare_gt(self, a, b);
}

static void exec_compare_ge(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_compare_ge(self, a, b);
}

/* ================= COUNTERS ================= */

static void exec_ctu(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_ctu(self, a, b);
}

static void exec_ctd(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_ctd(self, a, b);
}

static void exec_ctud(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_ctud(self, a, b);
}

/* ================= MEMORY ================= */

static void exec_mem_bool(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_mem_bool(self, a, b);
}

static void exec_mem_int(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_mem_int(self, a, b);
}

static void exec_mem_real(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_mem_real(self, a, b);
}

/* ================= FILTER ================= */

static void exec_analog_avg(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_analog_avg(self, a);
}

static void exec_filter_avg(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_filter_avg(self, a);
}

static void exec_ramp(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);

    plc_node_exec_ramp(self, a, dt_ms);
}

/* ================= MATH ================= */

static void exec_scale(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_scale(self, a);
}

static void exec_add(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_add(self, a, b);
}

static void exec_limit(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_limit(self, a);
}

static void exec_math_op(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_math_op(self, a, b);
}

/* ================= PID ================= */

static void exec_pid(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);

    plc_node_exec_pid(self, a, b, dt_ms);
}


static void exec_input_noop(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(self);
    UNUSED(a);
    UNUSED(b);
    UNUSED(dt_ms);
}

/* ================= IO ================= */

static void exec_do(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_do(self, a);
}

static void exec_ao(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_ao(self, a);
}

/* ================= MISC ================= */

static void exec_hyst(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_hyst(self, a);
}

static void exec_heartbeat(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(a);
    UNUSED(b);

    plc_node_exec_heartbeat(self, dt_ms);
}

static void exec_alarm_latch(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_alarm_latch(self, a, b);
}

static void exec_alarm_gen(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_alarm_gen(self, a);
}

static void exec_window_check(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_window_check(self, a);
}

static void exec_safe_output(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(dt_ms);

    plc_node_exec_safe_output(self, a, b);
}

/* ================= SPECIAL IO ================= */

static void exec_hsc_in(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(a);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_hsc_in(self);
}

static void exec_encoder_in(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b, uint32_t dt_ms)
{
    UNUSED(g);
    UNUSED(a);
    UNUSED(b);
    UNUSED(dt_ms);

    plc_node_exec_encoder_in(self);
}

/* ================= REGISTRY ================= */

PlcNodeExecFn plc_node_get_exec_fn(PlcNodeType type)
{
    switch (type) {

        case PLC_NODE_CONST_BOOL:      return exec_const_bool;
        case PLC_NODE_CONST_INT:       return exec_const_int;
        case PLC_NODE_CONST_FLOAT:     return exec_const_float;

        case PLC_NODE_MUX2:            return exec_mux2;
        case PLC_NODE_LOG:             return exec_log;

        case PLC_NODE_AND2:            return exec_and2;
        case PLC_NODE_OR2:             return exec_or2;
        case PLC_NODE_NOT:             return exec_not;
        case PLC_NODE_SR:              return exec_sr;

        case PLC_NODE_TON:             return exec_ton;
        case PLC_NODE_TOFF:            return exec_toff;
        case PLC_NODE_TP:              return exec_tp;
        case PLC_NODE_R_TRIG:          return exec_r_trig;
        case PLC_NODE_F_TRIG:          return exec_f_trig;

        case PLC_NODE_COMPARE_LT:      return exec_compare_lt;
        case PLC_NODE_COMPARE_GT:      return exec_compare_gt;
        case PLC_NODE_COMPARE_GE:      return exec_compare_ge;

        case PLC_NODE_CTU:             return exec_ctu;
        case PLC_NODE_CTD:             return exec_ctd;
        case PLC_NODE_CTUD:            return exec_ctud;

        case PLC_NODE_MEM_BOOL:        return exec_mem_bool;
        case PLC_NODE_MEM_INT:         return exec_mem_int;
        case PLC_NODE_MEM_REAL:        return exec_mem_real;

        case PLC_NODE_ANALOG_AVG:      return exec_analog_avg;
        case PLC_NODE_FILTER_AVG:      return exec_filter_avg;
        case PLC_NODE_RAMP:            return exec_ramp;

        case PLC_NODE_SCALE:           return exec_scale;
        case PLC_NODE_ADD:             return exec_add;
        case PLC_NODE_LIMIT:           return exec_limit;
        case PLC_NODE_MATH_OP:         return exec_math_op;

        case PLC_NODE_PID:             return exec_pid;

        case PLC_NODE_DIGITAL_IN:      return exec_input_noop;
        case PLC_NODE_AI_IN:           return exec_input_noop;
        case PLC_NODE_DIGITAL_OUT:     return exec_do;
        case PLC_NODE_AO:              return exec_ao;

        case PLC_NODE_HYST:            return exec_hyst;
        case PLC_NODE_HEARTBEAT:       return exec_heartbeat;
        case PLC_NODE_ALARM_LATCH:    return exec_alarm_latch;
        case PLC_NODE_ALARM_GEN:      return exec_alarm_gen;
        case PLC_NODE_WINDOW_CHECK:   return exec_window_check;
        case PLC_NODE_SAFE_OUTPUT:    return exec_safe_output;

        case PLC_NODE_HSC_IN:          return exec_hsc_in;
        case PLC_NODE_ENCODER_IN:     return exec_encoder_in;

        default:
            return 0;
    }
}