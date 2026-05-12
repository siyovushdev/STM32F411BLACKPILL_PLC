#pragma once

#include "friendly_plc/plc.h"

void plc_node_exec_ton(PlcNode* self, const PlcNode* a, uint32_t dt_ms);
void plc_node_exec_toff(PlcNode* self, const PlcNode* a, uint32_t dt_ms);
void plc_node_exec_tp(PlcNode* self, const PlcNode* a, uint32_t dt_ms);
void plc_node_exec_r_trig(PlcNode* self, const PlcNode* a);
void plc_node_exec_f_trig(PlcNode* self, const PlcNode* a);