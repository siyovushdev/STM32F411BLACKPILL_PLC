#include "friendly_plc/plc.h"
#include "friendly_plc/plc_port.h"
#include "friendly_plc/plc_log.h"
#include "friendly_plc/plc_error.h"

#include <stdint.h>
#include <stdbool.h>

static PlcError s_last_error = PLC_OK;

static PlcError plc_set_error(PlcError err)
{
    s_last_error = err;
    return err;
}

PlcError plc_get_last_error(void)
{
    return s_last_error;
}

static inline PlcValueType node_type(const PlcGraph* g, int16_t idx)
{
    if (!g) return PLC_VAL_FLOAT;
    if (idx < 0) return PLC_VAL_FLOAT;
    if ((uint16_t)idx >= g->nodeCount) return PLC_VAL_FLOAT;
    return (PlcValueType)g->nodes[idx].valueType;
}

static inline bool is_value_type_valid(uint8_t valueType)
{
    return valueType == PLC_VAL_BOOL ||
           valueType == PLC_VAL_INT ||
           valueType == PLC_VAL_FLOAT;
}

static inline bool is_bool_node(const PlcGraph* g, int16_t idx)
{
    return node_type(g, idx) == PLC_VAL_BOOL;
}

static inline bool is_numeric_type(PlcValueType t)
{
    return t == PLC_VAL_INT || t == PLC_VAL_FLOAT;
}

static inline bool is_numeric_node(const PlcGraph* g, int16_t idx)
{
    return is_numeric_type(node_type(g, idx));
}

/*
 * PLC-style compatibility:
 * INT and FLOAT are compatible because INT can be promoted to REAL/FLOAT.
 * BOOL is compatible only with BOOL.
 */
static inline bool types_compatible_nodes(const PlcGraph* g, int16_t a, int16_t b)
{
    PlcValueType ta = node_type(g, a);
    PlcValueType tb = node_type(g, b);

    if (ta == tb) return true;
    return is_numeric_type(ta) && is_numeric_type(tb);
}

static PlcError require_no_inputs(const PlcNode* n, uint16_t i, const char* name)
{
    if (n->inA != -1 || n->inB != -1) {
        PLC_LOGT(PLC_LOG_TAG, "validate: %s[%u] must not have inputs", name, (unsigned)i);
        return PLC_ERR_BAD_INPUT_REF;
    }

    return PLC_OK;
}

static PlcError require_in_a(const PlcNode* n, uint16_t i, const char* name)
{
    if (n->inA < 0) {
        PLC_LOGT(PLC_LOG_TAG, "validate: %s[%u] requires inA", name, (unsigned)i);
        return PLC_ERR_BAD_INPUT_REF;
    }

    return PLC_OK;
}

static PlcError require_in_ab(const PlcNode* n, uint16_t i, const char* name)
{
    if (n->inA < 0 || n->inB < 0) {
        PLC_LOGT(PLC_LOG_TAG, "validate: %s[%u] requires inA and inB", name, (unsigned)i);
        return PLC_ERR_BAD_INPUT_REF;
    }

    return PLC_OK;
}

static PlcError require_bool_input_a(const PlcGraph* g, const PlcNode* n, uint16_t i, const char* name)
{
    PlcError err = require_in_a(n, i, name);
    if (err != PLC_OK) return err;

    if (!is_bool_node(g, n->inA)) {
        PLC_LOGT(PLC_LOG_TAG, "validate: %s[%u] inA must be BOOL", name, (unsigned)i);
        return PLC_ERR_BAD_INPUT_TYPE;
    }

    return PLC_OK;
}

static PlcError require_numeric_input_a(const PlcGraph* g, const PlcNode* n, uint16_t i, const char* name)
{
    PlcError err = require_in_a(n, i, name);
    if (err != PLC_OK) return err;

    if (!is_numeric_node(g, n->inA)) {
        PLC_LOGT(PLC_LOG_TAG, "validate: %s[%u] inA must be INT or FLOAT", name, (unsigned)i);
        return PLC_ERR_BAD_INPUT_TYPE;
    }

    return PLC_OK;
}

static PlcError require_bool_inputs_ab(const PlcGraph* g, const PlcNode* n, uint16_t i, const char* name)
{
    PlcError err = require_in_ab(n, i, name);
    if (err != PLC_OK) return err;

    if (!is_bool_node(g, n->inA) || !is_bool_node(g, n->inB)) {
        PLC_LOGT(PLC_LOG_TAG, "validate: %s[%u] inputs must be BOOL", name, (unsigned)i);
        return PLC_ERR_BAD_INPUT_TYPE;
    }

    return PLC_OK;
}

static PlcError require_numeric_inputs_ab(const PlcGraph* g, const PlcNode* n, uint16_t i, const char* name)
{
    PlcError err = require_in_ab(n, i, name);
    if (err != PLC_OK) return err;

    if (!is_numeric_node(g, n->inA) || !is_numeric_node(g, n->inB)) {
        PLC_LOGT(PLC_LOG_TAG, "validate: %s[%u] inputs must be INT or FLOAT", name, (unsigned)i);
        return PLC_ERR_BAD_INPUT_TYPE;
    }

    return PLC_OK;
}

static PlcError require_hw_channel(int32_t ch, uint16_t count, uint16_t i, const char* name)
{
    if (ch < 0 || ch >= (int32_t)count) {
        PLC_LOGT(PLC_LOG_TAG,
                 "validate: %s[%u] bad channel=%d, count=%u",
                 name,
                 (unsigned)i,
                 (int)ch,
                 (unsigned)count);

        return PLC_ERR_BAD_HW_CHANNEL;
    }

    return PLC_OK;
}

static PlcError require_value_type(const PlcNode* n, uint16_t i, const char* name, PlcValueType expected)
{
    if (n->valueType != expected) {
        PLC_LOGT(PLC_LOG_TAG,
                 "validate: %s[%u] valueType must be %u, got=%u",
                 name,
                 (unsigned)i,
                 (unsigned)expected,
                 (unsigned)n->valueType);

        return PLC_ERR_BAD_VALUE_TYPE;
    }

    return PLC_OK;
}

static PlcError require_numeric_value_type(const PlcNode* n, uint16_t i, const char* name)
{
    if (n->valueType != PLC_VAL_INT && n->valueType != PLC_VAL_FLOAT) {
        PLC_LOGT(PLC_LOG_TAG,
                 "validate: %s[%u] valueType must be INT or FLOAT, got=%u",
                 name,
                 (unsigned)i,
                 (unsigned)n->valueType);

        return PLC_ERR_BAD_VALUE_TYPE;
    }

    return PLC_OK;
}

PlcError plc_validate_graph_ex(PlcGraph *g)
{
    s_last_error = PLC_OK;

    if (!g) {
        return plc_set_error(PLC_ERR_NULL);
    }

    if (g->nodeCount == 0) {
        PLC_LOGT(PLC_LOG_TAG, "validate: empty graph");
        return plc_set_error(PLC_ERR_EMPTY_GRAPH);
    }

    if (g->nodeCount > PLC_MAX_NODES) {
        PLC_LOGT(PLC_LOG_TAG,
                 "validate: nodeCount=%u > PLC_MAX_NODES=%u",
                 (unsigned)g->nodeCount,
                 (unsigned)PLC_MAX_NODES);

        return plc_set_error(PLC_ERR_TOO_MANY_NODES);
    }

    PlcPortHwInfo hw = plc_port_get_hw_info();

    if (g->cycleMs < PLC_CYCLE_MS_MIN) {
        g->cycleMs = PLC_CYCLE_MS_MIN;
    }

    if (g->cycleMs > PLC_CYCLE_MS_MAX) {
        g->cycleMs = PLC_CYCLE_MS_MAX;
    }

    for (uint16_t i = 0; i < g->nodeCount; i++) {
        PlcNode *n = &g->nodes[i];
        PlcError err = PLC_OK;

        if (n->id != i) {
            PLC_LOGT(PLC_LOG_TAG,
                     "validate: node[%u] bad id=%d",
                     (unsigned)i,
                     (int)n->id);

            return plc_set_error(PLC_ERR_BAD_NODE_ID);
        }

        /*
         * PLC_NODE_MAX is a sentinel, not a real executable node.
         */
        if (n->type >= PLC_NODE_MAX) {
            PLC_LOGT(PLC_LOG_TAG,
                     "validate: node[%u] bad type=%u",
                     (unsigned)i,
                     (unsigned)n->type);

            return plc_set_error(PLC_ERR_BAD_NODE_TYPE);
        }

        if (!is_value_type_valid(n->valueType)) {
            PLC_LOGT(PLC_LOG_TAG,
                     "validate: node[%u] bad valueType=%u",
                     (unsigned)i,
                     (unsigned)n->valueType);

            return plc_set_error(PLC_ERR_BAD_VALUE_TYPE);
        }

        /*
         * Graph must be topologically sorted.
         * Inputs may reference only already evaluated nodes.
         */
        if (n->inA >= 0 && (uint16_t)n->inA >= i) {
            PLC_LOGT(PLC_LOG_TAG,
                     "validate: node[%u] bad inA=%d",
                     (unsigned)i,
                     (int)n->inA);

            return plc_set_error(PLC_ERR_BAD_INPUT_REF);
        }

        if (n->inB >= 0 && (uint16_t)n->inB >= i) {
            PLC_LOGT(PLC_LOG_TAG,
                     "validate: node[%u] bad inB=%d",
                     (unsigned)i,
                     (int)n->inB);

            return plc_set_error(PLC_ERR_BAD_INPUT_REF);
        }

        switch (n->type) {
            case PLC_NODE_CONST_BOOL:
                err = require_no_inputs(n, i, "CONST_BOOL");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "CONST_BOOL", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_CONST_INT:
                err = require_no_inputs(n, i, "CONST_INT");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "CONST_INT", PLC_VAL_INT);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_CONST_FLOAT:
                err = require_no_inputs(n, i, "CONST_FLOAT");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "CONST_FLOAT", PLC_VAL_FLOAT);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_DIGITAL_IN:
                err = require_no_inputs(n, i, "DI");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_hw_channel(n->paramInt, hw.di_count, i, "DI");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "DI", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_DIGITAL_OUT:
                err = require_in_a(n, i, "DO");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_hw_channel(n->paramInt, hw.do_count, i, "DO");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "DO", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_AI_IN:
                err = require_no_inputs(n, i, "AI");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_hw_channel(n->paramInt, hw.ai_count, i, "AI");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_numeric_value_type(n, i, "AI");
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_AO:
                err = require_numeric_input_a(g, n, i, "AO");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_hw_channel(n->paramInt, hw.ao_count, i, "AO");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "AO", PLC_VAL_FLOAT);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_HSC_IN:
                err = require_no_inputs(n, i, "HSC");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_hw_channel(n->paramInt, hw.hsc_count, i, "HSC");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_numeric_value_type(n, i, "HSC");
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_ENCODER_IN:
                err = require_no_inputs(n, i, "ENCODER");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_hw_channel(n->paramInt, hw.encoder_count, i, "ENCODER");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_numeric_value_type(n, i, "ENCODER");
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_AND2:
            case PLC_NODE_OR2:
                err = require_bool_inputs_ab(g, n, i, "LOGIC");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "LOGIC", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_NOT:
            case PLC_NODE_R_TRIG:
            case PLC_NODE_F_TRIG:
                err = require_bool_input_a(g, n, i, "BOOL_1IN");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "BOOL_1IN", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_SR:
                if (n->inA < 0 && n->inB < 0) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: SR[%u] requires at least one input", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_REF);
                }
                if (n->inA >= 0 && !is_bool_node(g, n->inA)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: SR[%u] inA must be BOOL", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                if (n->inB >= 0 && !is_bool_node(g, n->inB)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: SR[%u] inB must be BOOL", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                err = require_value_type(n, i, "SR", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_TON:
            case PLC_NODE_TOFF:
            case PLC_NODE_TP:
                err = require_bool_input_a(g, n, i, "TIMER");
                if (err != PLC_OK) return plc_set_error(err);
                if (n->paramMs > 600000u) {
                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: TIMER[%u] paramMs too large=%u",
                             (unsigned)i,
                             (unsigned)n->paramMs);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                err = require_value_type(n, i, "TIMER", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_COMPARE_LT:
            case PLC_NODE_COMPARE_GT:
            case PLC_NODE_COMPARE_GE:
                err = require_in_ab(n, i, "CMP");
                if (err != PLC_OK) return plc_set_error(err);
                if (!types_compatible_nodes(g, n->inA, n->inB)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: CMP[%u] incompatible input types", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                if (!is_numeric_node(g, n->inA) || !is_numeric_node(g, n->inB)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: CMP[%u] inputs must be numeric", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                err = require_value_type(n, i, "CMP", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_MUX2:
                err = require_in_ab(n, i, "MUX2");
                if (err != PLC_OK) return plc_set_error(err);
                if (n->paramInt < 0 || n->paramInt >= (int32_t)i) {
                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: MUX2[%u] selector=%d invalid",
                             (unsigned)i,
                             (int)n->paramInt);
                    return plc_set_error(PLC_ERR_BAD_INPUT_REF);
                }
                if (!is_bool_node(g, (int16_t)n->paramInt)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: MUX2[%u] selector must be BOOL", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                if (!types_compatible_nodes(g, n->inA, n->inB)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: MUX2[%u] incompatible data input types", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                if (!types_compatible_nodes(g, n->inA, (int16_t)i) ||
                    !types_compatible_nodes(g, n->inB, (int16_t)i)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: MUX2[%u] output type incompatible with inputs", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_VALUE_TYPE);
                }
                break;

            case PLC_NODE_HYST:
                err = require_numeric_input_a(g, n, i, "HYST");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "HYST", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_SCALE:
                err = require_numeric_input_a(g, n, i, "SCALE");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "SCALE", PLC_VAL_FLOAT);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_ADD:
                err = require_numeric_inputs_ab(g, n, i, "ADD");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "ADD", PLC_VAL_FLOAT);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_LIMIT:
                err = require_numeric_input_a(g, n, i, "LIMIT");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "LIMIT", PLC_VAL_FLOAT);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_PID:
                err = require_numeric_inputs_ab(g, n, i, "PID");
                if (err != PLC_OK) return plc_set_error(err);
                if (n->paramInt < 0) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: PID[%u] Ki must be >= 0", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                /*
                 * Runtime supports BOOL/INT/FLOAT output conversion for PID.
                 */
                break;

            case PLC_NODE_ANALOG_AVG:
            case PLC_NODE_FILTER_AVG:
                err = require_numeric_input_a(g, n, i, "FILTER_AVG");
                if (err != PLC_OK) return plc_set_error(err);
                if (n->paramInt < 0 || n->paramInt > 1000) {
                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: FILTER_AVG[%u] alpha must be 0..1000, got=%d",
                             (unsigned)i,
                             (int)n->paramInt);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                err = require_value_type(n, i, "FILTER_AVG", PLC_VAL_FLOAT);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_RAMP:
                err = require_numeric_input_a(g, n, i, "RAMP");
                if (err != PLC_OK) return plc_set_error(err);
                if (n->paramInt < 0) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: RAMP[%u] rate must be >= 0", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                err = require_value_type(n, i, "RAMP", PLC_VAL_FLOAT);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_LOG:
                err = require_numeric_input_a(g, n, i, "LOG");
                if (err != PLC_OK) return plc_set_error(err);
                if (n->paramMs < 10u) {
                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: LOG[%u] paramMs must be >= 10",
                             (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                err = require_value_type(n, i, "LOG", PLC_VAL_FLOAT);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_CTU:
            case PLC_NODE_CTD:
                err = require_bool_input_a(g, n, i, "COUNTER");
                if (err != PLC_OK) return plc_set_error(err);
                if (n->inB >= 0 && !is_bool_node(g, n->inB)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: COUNTER[%u] reset input must be BOOL", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                if (n->paramInt < 0) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: COUNTER[%u] preset must be >= 0", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                if (n->valueType != PLC_VAL_INT && n->valueType != PLC_VAL_BOOL) {
                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: COUNTER[%u] valueType must be INT or BOOL",
                             (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_VALUE_TYPE);
                }
                break;

            case PLC_NODE_CTUD:
                if (n->inA < 0 && n->inB < 0) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: CTUD[%u] requires CU or CD", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_REF);
                }
                if (n->inA >= 0 && !is_bool_node(g, n->inA)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: CTUD[%u] CU must be BOOL", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                if (n->inB >= 0 && !is_bool_node(g, n->inB)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: CTUD[%u] CD must be BOOL", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                if (n->paramInt < 0) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: CTUD[%u] paramInt must be >= 0", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                if (n->valueType != PLC_VAL_INT && n->valueType != PLC_VAL_BOOL) {
                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: CTUD[%u] valueType must be INT or BOOL",
                             (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_VALUE_TYPE);
                }
                break;

            case PLC_NODE_WINDOW_CHECK:
                err = require_numeric_input_a(g, n, i, "WINDOW_CHECK");
                if (err != PLC_OK) return plc_set_error(err);
                if (n->paramInt < 0) {
                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: WINDOW_CHECK[%u] width must be >= 0",
                             (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                err = require_value_type(n, i, "WINDOW_CHECK", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_SAFE_OUTPUT:
                err = require_in_a(n, i, "SAFE_OUTPUT");
                if (err != PLC_OK) return plc_set_error(err);
                if (n->inB >= 0 && !is_bool_node(g, n->inB)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: SAFE_OUTPUT[%u] allow input must be BOOL", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                break;

            case PLC_NODE_ALARM_GEN:
                err = require_bool_input_a(g, n, i, "ALARM_GEN");
                if (err != PLC_OK) return plc_set_error(err);
                err = require_value_type(n, i, "ALARM_GEN", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_ALARM_LATCH:
                err = require_bool_input_a(g, n, i, "ALARM_LATCH");
                if (err != PLC_OK) return plc_set_error(err);
                if (n->inB >= 0 && !is_bool_node(g, n->inB)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: ALARM_LATCH[%u] ACK input must be BOOL", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                err = require_value_type(n, i, "ALARM_LATCH", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_HEARTBEAT:
                err = require_no_inputs(n, i, "HEARTBEAT");
                if (err != PLC_OK) return plc_set_error(err);
                if (n->paramMs == 0u) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: HEARTBEAT[%u] paramMs must be > 0", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                err = require_value_type(n, i, "HEARTBEAT", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_MEM_BOOL:
                if (n->paramInt < 0 || n->paramInt >= PLC_MEM_BOOL_COUNT) {
                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: MEM_BOOL[%u] index out of range=%d",
                             (unsigned)i,
                             (int)n->paramInt);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                if (n->inA >= 0 && !is_bool_node(g, n->inA)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: MEM_BOOL[%u] inA must be BOOL", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                if (n->inB >= 0 && !is_bool_node(g, n->inB)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: MEM_BOOL[%u] WE must be BOOL", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                err = require_value_type(n, i, "MEM_BOOL", PLC_VAL_BOOL);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_MEM_INT:
                if (n->paramInt < 0 || n->paramInt >= PLC_MEM_INT_COUNT) {
                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: MEM_INT[%u] index out of range=%d",
                             (unsigned)i,
                             (int)n->paramInt);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                if (n->inA >= 0 && node_type(g, n->inA) != PLC_VAL_INT) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: MEM_INT[%u] inA must be INT", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                if (n->inB >= 0 && !is_bool_node(g, n->inB)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: MEM_INT[%u] WE must be BOOL", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                err = require_value_type(n, i, "MEM_INT", PLC_VAL_INT);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_MEM_REAL:
                if (n->paramInt < 0 || n->paramInt >= PLC_MEM_REAL_COUNT) {
                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: MEM_REAL[%u] index out of range=%d",
                             (unsigned)i,
                             (int)n->paramInt);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                if (n->inA >= 0 && !is_numeric_node(g, n->inA)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: MEM_REAL[%u] inA must be numeric", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                if (n->inB >= 0 && !is_bool_node(g, n->inB)) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: MEM_REAL[%u] WE must be BOOL", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_TYPE);
                }
                err = require_value_type(n, i, "MEM_REAL", PLC_VAL_FLOAT);
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_MATH_OP:
                err = require_numeric_inputs_ab(g, n, i, "MATH_OP");
                if (err != PLC_OK) return plc_set_error(err);
                if (n->paramInt < 0 || n->paramInt > 3) {
                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: MATH_OP[%u] op must be 0..3, got=%d",
                             (unsigned)i,
                             (int)n->paramInt);
                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }
                err = require_numeric_value_type(n, i, "MATH_OP");
                if (err != PLC_OK) return plc_set_error(err);
                break;

            case PLC_NODE_MAX:
            default:
                PLC_LOGT(PLC_LOG_TAG,
                         "validate: node[%u] unsupported type=%u",
                         (unsigned)i,
                         (unsigned)n->type);
                return plc_set_error(PLC_ERR_BAD_NODE_TYPE);
        }
    }

    return plc_set_error(PLC_OK);
}

bool plc_validate_graph(PlcGraph* g)
{
    return plc_validate_graph_ex(g) == PLC_OK;
}
