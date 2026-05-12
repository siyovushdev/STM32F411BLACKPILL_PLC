#pragma once

#include "friendly_plc/plc.h"

#include <stdint.h>

typedef void (*PlcNodeExecFn)(
        PlcGraph* g,
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b,
        uint32_t dt_ms);

PlcNodeExecFn plc_node_get_exec_fn(PlcNodeType type);