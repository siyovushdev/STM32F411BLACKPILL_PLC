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

// --- validation helpers (PLC-style) ---
static inline PlcValueType node_type(const PlcGraph* g, int16_t idx) {
    if (!g) return PLC_VAL_FLOAT;
    if (idx < 0) return PLC_VAL_FLOAT;
    if ((uint16_t)idx >= g->nodeCount) return PLC_VAL_FLOAT;
    return g->nodes[idx].valueType;
}

static bool is_bool_node32(const PlcGraph *g, int idx) {
    if (idx < 0 || idx >= (int)g->nodeCount) return false;
    return g->nodes[idx].valueType == PLC_VAL_BOOL;
}

static inline bool is_bool_node16(const PlcGraph* g, int16_t idx) {
    return node_type(g, idx) == PLC_VAL_BOOL;
}

// FLOAT и INT считаем совместимыми (как в PLC: INT может быть приведён к REAL)
static inline bool types_compatible_nodes(const PlcGraph* g, int16_t a, int16_t b) {
    PlcValueType ta = node_type(g, a);
    PlcValueType tb = node_type(g, b);

    if (ta == tb) return true;

    const bool na = (ta == PLC_VAL_INT) || (ta == PLC_VAL_FLOAT);
    const bool nb = (tb == PLC_VAL_INT) || (tb == PLC_VAL_FLOAT);
    if (na && nb) return true;

    return false;
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

    // clamp cycle
    if (g->cycleMs < PLC_CYCLE_MS_MIN) {
        g->cycleMs = PLC_CYCLE_MS_MIN;
    }

    if (g->cycleMs > PLC_CYCLE_MS_MAX) {
        g->cycleMs = PLC_CYCLE_MS_MAX;
    }

    for (uint16_t i = 0; i < g->nodeCount; i++) {

        PlcNode *n = &g->nodes[i];

        // -------------------------------------------------
        // ID
        // -------------------------------------------------

        if (n->id != i) {

            PLC_LOGT(PLC_LOG_TAG,
                     "validate: node[%u] bad id=%d",
                     (unsigned)i,
                     (int)n->id);

            return plc_set_error(PLC_ERR_BAD_NODE_ID);
        }

        // -------------------------------------------------
        // TYPE
        // -------------------------------------------------

        if (n->type > PLC_NODE_MAX) {

            PLC_LOGT(PLC_LOG_TAG,
                     "validate: node[%u] bad type=%u",
                     (unsigned)i,
                     (unsigned)n->type);

            return plc_set_error(PLC_ERR_BAD_NODE_TYPE);
        }

        // -------------------------------------------------
        // INPUT REFS
        // -------------------------------------------------

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

        // -------------------------------------------------
        // TYPE-SPECIFIC VALIDATION
        // -------------------------------------------------

        switch (n->type) {

            // =============================================
            // DIGITAL IN
            // =============================================

            case PLC_NODE_DIGITAL_IN:

                if (n->inA != -1 || n->inB != -1) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: DI[%u] must not have inputs",
                             (unsigned)i);

                    return plc_set_error(PLC_ERR_BAD_INPUT_REF);
                }

                if (n->paramInt < 0 ||
                    n->paramInt >= hw.di_count) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: DI[%u] bad channel=%d",
                             (unsigned)i,
                             (int)n->paramInt);

                    return plc_set_error(PLC_ERR_BAD_HW_CHANNEL);
                }

                if (n->valueType != PLC_VAL_BOOL) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: DI[%u] valueType must be BOOL",
                             (unsigned)i);

                    return plc_set_error(PLC_ERR_BAD_VALUE_TYPE);
                }

                break;

                // =============================================
                // DIGITAL OUT
                // =============================================

            case PLC_NODE_DIGITAL_OUT:

                if (n->inA < 0) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: DO[%u] requires inA",
                             (unsigned)i);

                    return plc_set_error(PLC_ERR_BAD_INPUT_REF);
                }

                if (n->paramInt < 0 ||
                    n->paramInt >= hw.do_count) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: DO[%u] bad channel=%d",
                             (unsigned)i,
                             (int)n->paramInt);

                    return plc_set_error(PLC_ERR_BAD_HW_CHANNEL);
                }

                break;

                // =============================================
                // AI
                // =============================================

            case PLC_NODE_AI_IN:

                if (n->paramInt < 0 ||
                    n->paramInt >= hw.ai_count) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: AI[%u] bad channel=%d",
                             (unsigned)i,
                             (int)n->paramInt);

                    return plc_set_error(PLC_ERR_BAD_HW_CHANNEL);
                }

                break;

                // =============================================
                // AO
                // =============================================

            case PLC_NODE_AO:

                if (n->inA < 0) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: AO[%u] requires inA",
                             (unsigned)i);

                    return plc_set_error(PLC_ERR_BAD_INPUT_REF);
                }

                if (n->paramInt < 0 ||
                    n->paramInt >= hw.ao_count) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: AO[%u] bad channel=%d",
                             (unsigned)i,
                             (int)n->paramInt);

                    return plc_set_error(PLC_ERR_BAD_HW_CHANNEL);
                }

                break;

                // =============================================
                // LOGIC
                // =============================================

            case PLC_NODE_AND2:
            case PLC_NODE_OR2:

                if (n->inA < 0 || n->inB < 0) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: LOGIC[%u] missing inputs",
                             (unsigned)i);

                    return plc_set_error(PLC_ERR_BAD_INPUT_REF);
                }

                break;

                // =============================================
                // TIMERS
                // =============================================

            case PLC_NODE_TON:
            case PLC_NODE_TOFF:
            case PLC_NODE_TP:

                if (n->inA < 0) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: TIMER[%u] requires inA",
                             (unsigned)i);

                    return plc_set_error(PLC_ERR_BAD_INPUT_REF);
                }

                if (n->paramMs > 600000u) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: TIMER[%u] paramMs too large=%u",
                             (unsigned)i,
                             (unsigned)n->paramMs);

                    return plc_set_error(PLC_ERR_BAD_PARAM);
                }

                break;

                // =============================================
                // COMPARE
                // =============================================

            case PLC_NODE_COMPARE_LT:
            case PLC_NODE_COMPARE_GT:
            case PLC_NODE_COMPARE_GE:

                if (n->inA < 0 || n->inB < 0) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: CMP[%u] missing inputs",
                             (unsigned)i);

                    return plc_set_error(PLC_ERR_BAD_INPUT_REF);
                }

                break;

                // =============================================
                // CONST
                // =============================================

            case PLC_NODE_CONST_BOOL:
            case PLC_NODE_CONST_INT:
            case PLC_NODE_CONST_FLOAT:

                if (n->inA != -1 || n->inB != -1) {

                    PLC_LOGT(PLC_LOG_TAG,
                             "validate: CONST[%u] must not have inputs",
                             (unsigned)i);

                    return plc_set_error(PLC_ERR_BAD_INPUT_REF);
                }

                break;
            case PLC_NODE_HSC_IN:
                if (n->inA != -1 || n->inB != -1) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: HSC[%u] must not have inputs", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_REF);
                }

                if (n->paramInt < 0 || n->paramInt >= hw.hsc_count) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: HSC[%u] bad channel=%d", (unsigned)i, (int)n->paramInt);
                    return plc_set_error(PLC_ERR_BAD_HW_CHANNEL);
                }

                if (n->valueType != PLC_VAL_INT && n->valueType != PLC_VAL_FLOAT) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: HSC[%u] valueType must be INT or FLOAT", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_VALUE_TYPE);
                }
                break;

            case PLC_NODE_ENCODER_IN:
                if (n->inA != -1 || n->inB != -1) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: ENCODER[%u] must not have inputs", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_INPUT_REF);
                }

                if (n->paramInt < 0 || n->paramInt >= hw.encoder_count) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: ENCODER[%u] bad channel=%d", (unsigned)i, (int)n->paramInt);
                    return plc_set_error(PLC_ERR_BAD_HW_CHANNEL);
                }

                if (n->valueType != PLC_VAL_INT && n->valueType != PLC_VAL_FLOAT) {
                    PLC_LOGT(PLC_LOG_TAG, "validate: ENCODER[%u] valueType must be INT or FLOAT", (unsigned)i);
                    return plc_set_error(PLC_ERR_BAD_VALUE_TYPE);
                }
                break;
            default:
                break;
        }
    }

    return plc_set_error(PLC_OK);
}

bool plc_validate_graph(PlcGraph* g)
{
    return plc_validate_graph_ex(g) == PLC_OK;
}