#pragma once

#include "friendly_plc/plc.h"

void plc_node_exec_hyst(
        PlcNode* self,
        const PlcNode* a);

void plc_node_exec_heartbeat(
        PlcNode* self,
        uint32_t dt_ms);

void plc_node_exec_alarm_latch(
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b);

void plc_node_exec_alarm_gen(
        PlcNode* self,
        const PlcNode* a);

void plc_node_exec_window_check(
        PlcNode* self,
        const PlcNode* a);

void plc_node_exec_safe_output(
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b);