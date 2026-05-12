#include "friendly_plc/plc.h"
#include "friendly_plc/plc_port.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    plc_mem_init();

    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 3;

    g.nodes[0].id = 0;
    g.nodes[0].type = PLC_NODE_CONST_BOOL;
    g.nodes[0].valueType = PLC_VAL_BOOL;
    g.nodes[0].inA = -1;
    g.nodes[0].inB = -1;
    g.nodes[0].paramInt = 1;

    g.nodes[1].id = 1;
    g.nodes[1].type = PLC_NODE_MEM_BOOL;
    g.nodes[1].valueType = PLC_VAL_BOOL;
    g.nodes[1].inA = 0;
    g.nodes[1].inB = -1;
    g.nodes[1].paramInt = 0;

    g.nodes[2].id = 2;
    g.nodes[2].type = PLC_NODE_DIGITAL_OUT;
    g.nodes[2].valueType = PLC_VAL_BOOL;
    g.nodes[2].inA = 1;
    g.nodes[2].inB = -1;
    g.nodes[2].paramInt = 0;

    if (!plc_upload_graph(&g)) {
        printf("TEST FAILED: upload graph\n");
        return 1;
    }

    if (!plc_request_activate_graph()) {
        printf("TEST FAILED: activate graph\n");
        return 1;
    }

    uint32_t now = plc_port_now_ms();

    for (int i = 0; i < 3; i++) {
        now += 10;
        plc_tick(now);
    }

    if (!plc_mem_get_bool(0)) {
        printf("TEST FAILED: MEM_BOOL[0] must be true\n");
        return 1;
    }

    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must be true\n");
        return 1;
    }

    printf("TEST OK: CONST_BOOL -> MEM_BOOL[0] -> DO0\n");
    return 0;
}