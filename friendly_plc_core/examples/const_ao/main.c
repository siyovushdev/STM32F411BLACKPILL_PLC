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
    g.nodes[0].type = PLC_NODE_CONST_FLOAT;
    g.nodes[0].valueType = PLC_VAL_FLOAT;
    g.nodes[0].inA = -1;
    g.nodes[0].inB = -1;
    g.nodes[0].paramFloat = 55.0f;

    g.nodes[1].id = 1;
    g.nodes[1].type = PLC_NODE_AO;
    g.nodes[1].valueType = PLC_VAL_FLOAT;
    g.nodes[1].inA = 0;
    g.nodes[1].inB = -1;
    g.nodes[1].paramInt = 0;

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

    float ao = plc_port_test_get_ao_percent(0);

    if (ao < 54.9f || ao > 55.1f) {
        printf("TEST FAILED: AO0 expected 55.0, got %.3f\n", ao);
        return 1;
    }

    printf("TEST OK: CONST_FLOAT -> AO0 = %.1f%%\n", ao);
    return 0;
}