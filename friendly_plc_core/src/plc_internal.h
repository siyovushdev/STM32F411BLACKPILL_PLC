#pragma once

#include "friendly_plc/plc.h"

#include <stdint.h>

void plc_refresh_inputs_hw(uint32_t nowMs, PlcGraph* g);
void plc_graph_step(PlcGraph* g, uint32_t dt_ms);
void plc_io_reset_runtime(void);