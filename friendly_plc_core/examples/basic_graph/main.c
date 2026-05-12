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
    g.nodeCount = 2;

    g.nodes[0].id = 0;
    g.nodes[0].type = PLC_NODE_CONST_BOOL;
    g.nodes[0].valueType = PLC_VAL_BOOL;
    g.nodes[0].inA = -1;
    g.nodes[0].inB = -1;
    g.nodes[0].paramInt = 1;

    g.nodes[1].id = 1;
    g.nodes[1].type = PLC_NODE_DIGITAL_OUT;
    g.nodes[1].valueType = PLC_VAL_BOOL;
    g.nodes[1].inA = 0;
    g.nodes[1].inB = -1;
    g.nodes[1].paramInt = 0;

    if (!plc_upload_graph(&g)) {
        printf("plc_upload_graph failed\n");
        return 1;
    }

    if (!plc_request_activate_graph()) {
        printf("plc_request_activate_graph failed\n");
        return 1;
    }

    for (int i = 0; i < 5; i++) {
        plc_tick(plc_port_now_ms());
    }

#ifdef FRIENDLY_PLC_ENABLE_TEST_API
    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 is false\n");
        return 1;
    }
#endif
    printf("TEST OK: DO0 is true\n");

    printf("PLC basic example finished\n");
    return 0;
}