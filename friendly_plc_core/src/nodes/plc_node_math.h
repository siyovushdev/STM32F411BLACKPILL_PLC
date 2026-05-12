#pragma once

#include "friendly_plc/plc.h"

void plc_node_exec_scale(PlcNode* self, const PlcNode* a);
void plc_node_exec_add(PlcNode* self, const PlcNode* a, const PlcNode* b);
void plc_node_exec_limit(PlcNode* self, const PlcNode* a);
void plc_node_exec_math_op(PlcNode* self, const PlcNode* a, const PlcNode* b);