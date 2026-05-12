#pragma once

#include "friendly_plc/plc_types.h"
#include "friendly_plc/plc_error.h"
#include "friendly_plc/plc_safety.h"
#include "friendly_plc/plc_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

extern PlcGraph g_activeGraph;
extern PlcGraph g_stagingGraph;

extern bool g_activeGraphValid;
extern bool g_stagingGraphValid;

extern volatile bool g_needSwapGraph;

bool plc_upload_graph(const PlcGraph* src);
bool plc_request_activate_graph(void);

bool plc_force_output(uint16_t nodeIndex, bool value, uint32_t holdMs);
bool plc_release_output(uint16_t nodeIndex);

void plc_tick(uint32_t nowMs);

void plc_graph_reset_runtime(PlcGraph* g);
bool plc_validate_graph(PlcGraph* g);

PlcError plc_validate_graph_ex(PlcGraph* g);
PlcError plc_get_last_error(void);

#ifdef __cplusplus
}
#endif