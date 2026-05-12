#pragma once

#include "friendly_plc/plc.h"

void plc_node_exec_const_bool(PlcNode* self);
void plc_node_exec_const_int(PlcNode* self);
void plc_node_exec_const_float(PlcNode* self);

void plc_node_exec_mux2(PlcGraph* g, PlcNode* self, const PlcNode* a, const PlcNode* b);

void plc_node_exec_log(PlcNode* self, const PlcNode* a, uint32_t dt_ms);