#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLC_OK = 0,

    PLC_ERR_NULL,
    PLC_ERR_EMPTY_GRAPH,
    PLC_ERR_TOO_MANY_NODES,
    PLC_ERR_BAD_NODE_ID,
    PLC_ERR_BAD_NODE_TYPE,
    PLC_ERR_BAD_INPUT_REF,
    PLC_ERR_BAD_INPUT_TYPE,
    PLC_ERR_BAD_HW_CHANNEL,
    PLC_ERR_BAD_PARAM,
    PLC_ERR_BAD_VALUE_TYPE,
    PLC_ERR_STAGING_INVALID,
    PLC_ERR_ACTIVE_GRAPH_MISSING,
    PLC_ERR_NODE_INDEX_OUT_OF_RANGE,
    PLC_ERR_NODE_NOT_OUTPUT
} PlcError;

const char* plc_error_to_string(PlcError err);

#ifdef __cplusplus
}
#endif