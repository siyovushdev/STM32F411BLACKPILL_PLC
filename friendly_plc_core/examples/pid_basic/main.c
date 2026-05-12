#include "friendly_plc/plc.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 4;

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_CONST_FLOAT, .valueType=PLC_VAL_FLOAT, .inA=-1, .inB=-1, .paramFloat=100.0f };
    g.nodes[1] = (PlcNode){ .id=1, .type=PLC_NODE_CONST_FLOAT, .valueType=PLC_VAL_FLOAT, .inA=-1, .inB=-1, .paramFloat=0.0f };
    g.nodes[2] = (PlcNode){
        .id=2,
        .type=PLC_NODE_PID,
        .valueType=PLC_VAL_FLOAT,
        .inA=0,
        .inB=1,
        .paramFloat=1.0f,
        .paramInt=0,
        .paramMs=0,
        .flags=PLC_NODE_FLAG_PID_CLAMP_0_100
    };
    g.nodes[3] = (PlcNode){ .id=3, .type=PLC_NODE_AO, .valueType=PLC_VAL_FLOAT, .inA=2, .inB=-1, .paramInt=0 };

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    plc_tick(10);

    float out1 = g_activeGraph.nodes[2].out.f;

    if (out1 < 99.9f || out1 > 100.1f) {
        printf("TEST FAILED: PID output expected near 100, got %.3f\n", out1);
        return 1;
    }

    printf("TEST OK: PID basic clamp output %.3f\n", out1);
    return 0;
}
