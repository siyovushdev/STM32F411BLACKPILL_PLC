#pragma once

#include "friendly_plc/plc.h"

void plc_node_exec_do(
        PlcNode* self,
        const PlcNode* a);

void plc_node_exec_ao(
        PlcNode* self,
        const PlcNode* a);