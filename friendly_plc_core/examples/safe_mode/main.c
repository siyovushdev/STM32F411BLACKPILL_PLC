#include "friendly_plc/plc.h"
#include "friendly_plc/plc_port.h"
#include "friendly_plc/plc_runtime.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    plc_runtime_init();
    plc_mem_init();

    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 2;

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_CONST_BOOL, .valueType=PLC_VAL_BOOL, .inA=-1, .inB=-1, .paramInt=1 };
    g.nodes[1] = (PlcNode){ .id=1, .type=PLC_NODE_DIGITAL_OUT, .valueType=PLC_VAL_BOOL, .inA=0, .inB=-1, .paramInt=0 };

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    uint32_t now = plc_port_now_ms();

    now += 10;
    plc_tick(now);

    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must be true before fault\n");
        return 1;
    }

    plc_runtime_enter_fault(PLC_RUNTIME_FAULT_BAD_NODE_TYPE);

    now += 10;
    plc_tick(now);

    if (plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must be false in safe mode\n");
        return 1;
    }

    if (!plc_runtime_is_faulted()) {
        printf("TEST FAILED: runtime must be faulted\n");
        return 1;
    }

    plc_runtime_reset_fault();

    now += 10;
    plc_tick(now);

    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: DO0 must return true after fault reset\n");
        return 1;
    }

    printf("TEST OK: safe mode resets outputs\n");
    return 0;
}