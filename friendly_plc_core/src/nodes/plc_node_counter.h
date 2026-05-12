#pragma once

#include "friendly_plc/plc.h"

void plc_node_exec_ctu(PlcNode* self, const PlcNode* a, const PlcNode* b);
void plc_node_exec_ctd(PlcNode* self, const PlcNode* a, const PlcNode* b);
void plc_node_exec_ctud(PlcNode* self, const PlcNode* a, const PlcNode* b);

