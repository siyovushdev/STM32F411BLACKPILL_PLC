#include "friendly_plc/plc.h"
#include "friendly_plc/plc_port.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 4;

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_CONST_BOOL, .valueType=PLC_VAL_BOOL, .inA=-1, .inB=-1, .paramInt=1 };
    g.nodes[1] = (PlcNode){ .id=1, .type=PLC_NODE_DIGITAL_IN, .valueType=PLC_VAL_BOOL, .inA=-1, .inB=-1, .paramInt=0 };
    g.nodes[2] = (PlcNode){ .id=2, .type=PLC_NODE_SAFE_OUTPUT, .valueType=PLC_VAL_BOOL, .inA=0, .inB=1, .paramInt=0 };
    g.nodes[3] = (PlcNode){ .id=3, .type=PLC_NODE_DIGITAL_OUT, .valueType=PLC_VAL_BOOL, .inA=2, .inB=-1, .paramInt=0 };

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    uint32_t now = plc_port_now_ms();

    plc_port_test_set_di(0, true);
    for (int i = 0; i < 5; i++) {
        now += 10;
        plc_tick(now);
    }

    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: safe output should pass normal value when allow=true\n");
        return 1;
    }

    plc_port_test_set_di(0, false);
    for (int i = 0; i < 5; i++) {
        now += 10;
        plc_tick(now);
    }

    if (plc_port_test_get_do(0)) {
        printf("TEST FAILED: safe output should output safe=false when allow=false\n");
        return 1;
    }

    printf("TEST OK: SAFE_OUTPUT\n");
    return 0;
}
