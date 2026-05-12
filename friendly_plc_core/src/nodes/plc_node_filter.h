#pragma once

#include "friendly_plc/plc.h"

void plc_node_exec_analog_avg(PlcNode* self, const PlcNode* a);
void plc_node_exec_filter_avg(PlcNode* self, const PlcNode* a);
void plc_node_exec_ramp(PlcNode* self, const PlcNode* a, uint32_t dt_ms);