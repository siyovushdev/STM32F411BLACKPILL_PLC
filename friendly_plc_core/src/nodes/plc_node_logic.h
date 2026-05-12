#pragma once

#include "friendly_plc/plc.h"

void plc_node_exec_and2(
        PlcNode *self,
        const PlcNode *a,
        const PlcNode *b);

void plc_node_exec_or2(
        PlcNode *self,
        const PlcNode *a,
        const PlcNode *b);

void plc_node_exec_not(
        PlcNode *self,
        const PlcNode *a);

void plc_node_exec_sr(
        PlcNode* self,
        const PlcNode* a,
        const PlcNode* b);