#include "friendly_plc/plc_error.h"

const char* plc_error_to_string(PlcError err)
{
    switch (err) {
        case PLC_OK: return "OK";
        case PLC_ERR_NULL: return "NULL pointer";
        case PLC_ERR_EMPTY_GRAPH: return "empty graph";
        case PLC_ERR_TOO_MANY_NODES: return "too many nodes";
        case PLC_ERR_BAD_NODE_ID: return "bad node id";
        case PLC_ERR_BAD_NODE_TYPE: return "bad node type";
        case PLC_ERR_BAD_INPUT_REF: return "bad input reference";
        case PLC_ERR_BAD_INPUT_TYPE: return "bad input type";
        case PLC_ERR_BAD_HW_CHANNEL: return "bad hardware channel";
        case PLC_ERR_BAD_PARAM: return "bad parameter";
        case PLC_ERR_BAD_VALUE_TYPE: return "bad value type";
        case PLC_ERR_STAGING_INVALID: return "staging graph invalid";
        case PLC_ERR_ACTIVE_GRAPH_MISSING: return "active graph missing";
        case PLC_ERR_NODE_INDEX_OUT_OF_RANGE: return "node index out of range";
        case PLC_ERR_NODE_NOT_OUTPUT: return "node is not output";
        default: return "unknown error";
    }
}