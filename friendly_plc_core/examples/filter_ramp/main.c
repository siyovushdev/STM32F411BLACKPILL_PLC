#include "friendly_plc/plc.h"

#include <stdio.h>
#include <string.h>

static int test_filter(void)
{
    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 2;

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_CONST_FLOAT, .valueType=PLC_VAL_FLOAT, .inA=-1, .inB=-1, .paramFloat=100.0f };
    g.nodes[1] = (PlcNode){ .id=1, .type=PLC_NODE_FILTER_AVG, .valueType=PLC_VAL_FLOAT, .inA=0, .inB=-1, .paramInt=500 };

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    plc_tick(10);
    float y1 = g_activeGraph.nodes[1].out.f;

    plc_tick(20);
    float y2 = g_activeGraph.nodes[1].out.f;

    if (y1 < 99.9f || y2 < 99.9f) {
        printf("TEST FAILED: FILTER_AVG should initialize to input\n");
        return 1;
    }

    return 0;
}

static int test_ramp(void)
{
    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 2;

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_CONST_FLOAT, .valueType=PLC_VAL_FLOAT, .inA=-1, .inB=-1, .paramFloat=100.0f };
    g.nodes[1] = (PlcNode){ .id=1, .type=PLC_NODE_RAMP, .valueType=PLC_VAL_FLOAT, .inA=0, .inB=-1, .paramInt=10000 }; // 10 units/sec

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    plc_tick(10);

    // First tick initializes to input according to current runtime behavior.
    if (g_activeGraph.nodes[1].out.f < 99.9f) {
        printf("TEST FAILED: RAMP should initialize to input\n");
        return 1;
    }

    return 0;
}

int main(void)
{
    if (test_filter() != 0) return 1;
    if (test_ramp() != 0) return 1;

    printf("TEST OK: FILTER_AVG and RAMP\n");
    return 0;
}
