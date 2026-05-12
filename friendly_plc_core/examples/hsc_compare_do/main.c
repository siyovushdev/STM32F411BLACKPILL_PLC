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
    g.nodeCount = 4;

    g.nodes[0].id = 0;
    g.nodes[0].type = PLC_NODE_ENCODER_IN;
    g.nodes[0].valueType = PLC_VAL_INT;
    g.nodes[0].inA = -1;
    g.nodes[0].inB = -1;
    g.nodes[0].paramInt = 0;

    g.nodes[1].id = 1;
    g.nodes[1].type = PLC_NODE_CONST_INT;
    g.nodes[1].valueType = PLC_VAL_INT;
    g.nodes[1].inA = -1;
    g.nodes[1].inB = -1;
    g.nodes[1].paramInt = 100;

    g.nodes[2].id = 2;
    g.nodes[2].type = PLC_NODE_COMPARE_GT;
    g.nodes[2].valueType = PLC_VAL_BOOL;
    g.nodes[2].inA = 0;
    g.nodes[2].inB = 1;

    g.nodes[3].id = 3;
    g.nodes[3].type = PLC_NODE_DIGITAL_OUT;
    g.nodes[3].valueType = PLC_VAL_BOOL;
    g.nodes[3].inA = 2;
    g.nodes[3].inB = -1;
    g.nodes[3].paramInt = 0;

    if (!plc_upload_graph(&g)) {
        printf("TEST FAILED: upload graph\n");
        return 1;
    }

    if (!plc_request_activate_graph()) {
        printf("TEST FAILED: activate graph\n");
        return 1;
    }

    uint32_t now = plc_port_now_ms();

    plc_port_test_set_encoder(0, 50);

    for (int i = 0; i < 3; i++) {
        now += 10;
        plc_tick(now);
    }

    if (plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must be false when ENCODER0=50\n");
        return 1;
    }

    plc_port_test_set_encoder(0, 150);

    for (int i = 0; i < 3; i++) {
        now += 10;
        plc_tick(now);
    }

    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must be true when ENCODER0=150\n");
        return 1;
    }

    printf("TEST OK: ENCODER0 -> COMPARE_GT -> DO0\n");
    return 0;
}