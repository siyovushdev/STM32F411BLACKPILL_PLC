#include "friendly_plc/plc.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 3;

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_CONST_INT, .valueType=PLC_VAL_INT, .inA=-1, .inB=-1, .paramInt=20 };
    g.nodes[1] = (PlcNode){ .id=1, .type=PLC_NODE_CONST_INT, .valueType=PLC_VAL_INT, .inA=-1, .inB=-1, .paramInt=10 };

    g.nodes[2] = (PlcNode){
        .id=2,
        .type=PLC_NODE_MATH_OP,
        .valueType=PLC_VAL_INT,
        .inA=0,
        .inB=1,
        .paramInt=0
    };

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    plc_tick(10);

    if (g_activeGraph.nodes[2].out.i != 30) {
        printf("TEST FAILED: ADD failed\n");
        return 1;
    }

    printf("TEST OK: MATH_OP ADD\n");
    return 0;
}
