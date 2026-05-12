#include "friendly_plc/plc.h"

#include <stdio.h>
#include <string.h>

static int test_window(void)
{
    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 2;

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_CONST_FLOAT, .valueType=PLC_VAL_FLOAT, .inA=-1, .inB=-1, .paramFloat=10.0f };
    g.nodes[1] = (PlcNode){ .id=1, .type=PLC_NODE_WINDOW_CHECK, .valueType=PLC_VAL_BOOL, .inA=0, .inB=-1, .paramFloat=10.0f, .paramInt=2000 };

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    plc_tick(10);

    if (!g_activeGraph.nodes[1].out.b) {
        printf("TEST FAILED: WINDOW_CHECK should be true\n");
        return 1;
    }

    return 0;
}

static int test_hyst(void)
{
    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 2;

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_CONST_FLOAT, .valueType=PLC_VAL_FLOAT, .inA=-1, .inB=-1, .paramFloat=11.0f };
    g.nodes[1] = (PlcNode){ .id=1, .type=PLC_NODE_HYST, .valueType=PLC_VAL_BOOL, .inA=0, .inB=-1, .paramFloat=10.0f, .paramInt=2000 };

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    plc_tick(10);

    if (!g_activeGraph.nodes[1].out.b) {
        printf("TEST FAILED: HYST should switch on at hi threshold\n");
        return 1;
    }

    return 0;
}

int main(void)
{
    if (test_window() != 0) return 1;
    if (test_hyst() != 0) return 1;

    printf("TEST OK: WINDOW_CHECK and HYST\n");
    return 0;
}
