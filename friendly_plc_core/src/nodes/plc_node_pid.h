#pragma once

#include "friendly_plc/plc.h"

void plc_node_exec_pid(
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b,
        uint32_t dt_ms);