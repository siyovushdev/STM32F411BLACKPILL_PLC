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

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_CONST_BOOL, .valueType=PLC_VAL_BOOL, .inA=-1, .inB=-1, .paramInt=0 };
    g.nodes[1] = (PlcNode){ .id=1, .type=PLC_NODE_DIGITAL_OUT, .valueType=PLC_VAL_BOOL, .inA=0, .inB=-1, .paramInt=0 };

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    uint32_t now = plc_port_now_ms();

    now += 10;
    plc_tick(now);

    if (plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must be false initially\n");
        return 1;
    }

    if (!plc_force_output(1, true, 0)) {
        printf("TEST FAILED: force_output failed\n");
        return 1;
    }

    now += 10;
    plc_tick(now);

    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must be true after force\n");
        return 1;
    }

    if (!plc_release_output(1)) {
        printf("TEST FAILED: release_output failed\n");
        return 1;
    }

    now += 10;
    plc_tick(now);

    if (plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must return false after release\n");
        return 1;
    }

    printf("TEST OK: force/release DO\n");
    return 0;
}
