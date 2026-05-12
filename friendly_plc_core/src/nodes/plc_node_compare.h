#pragma once

#include "friendly_plc/plc.h"

void plc_node_exec_compare_lt(PlcNode* self, const PlcNode* a, const PlcNode* b);
void plc_node_exec_compare_gt(PlcNode* self, const PlcNode* a, const PlcNode* b);
void plc_node_exec_compare_ge(PlcNode* self, const PlcNode* a, const PlcNode* b);