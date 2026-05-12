#pragma once

#include "friendly_plc/plc.h"

void plc_node_exec_mem_bool(PlcNode* self, const PlcNode* a, const PlcNode* b);
void plc_node_exec_mem_int(PlcNode* self, const PlcNode* a, const PlcNode* b);
void plc_node_exec_mem_real(PlcNode* self, const PlcNode* a, const PlcNode* b);