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

    // Node 0: DI0
    g.nodes[0].id = 0;
    g.nodes[0].type = PLC_NODE_DIGITAL_IN;
    g.nodes[0].valueType = PLC_VAL_BOOL;
    g.nodes[0].inA = -1;
    g.nodes[0].inB = -1;
    g.nodes[0].paramInt = 0;

    // Node 1: TON 100 ms
    g.nodes[1].id = 1;
    g.nodes[1].type = PLC_NODE_TON;
    g.nodes[1].valueType = PLC_VAL_BOOL;
    g.nodes[1].inA = 0;
    g.nodes[1].inB = -1;
    g.nodes[1].paramMs = 100;

    // Node 2: DO0
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

    plc_port_test_set_di(0, false);

    for (int i = 0; i < 5; i++) {
        now += 10;
        plc_tick(now);
    }

    if (plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must be false before DI0 true\n");
        return 1;
    }

    plc_port_test_set_di(0, true);

    for (int i = 0; i < 5; i++) {
        now += 10;
        plc_tick(now);
    }

    if (plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must still be false before TON elapsed\n");
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        now += 10;
        plc_tick(now);
    }

    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must be true after TON elapsed\n");
        return 1;
    }

    plc_port_test_set_di(0, false);

    for (int i = 0; i < 5; i++) {
        now += 10;
        plc_tick(now);
    }

    if (plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must be false after DI0 false\n");
        return 1;
    }

    printf("TEST OK: DI0 -> TON -> DO0\n");
    return 0;
}