#include "friendly_plc/plc.h"
#include "friendly_plc/plc_port.h"

#include <stdio.h>
#include <string.h>

static void build_graph(PlcGraph* g, bool value)
{
    memset(g, 0, sizeof(*g));

    g->cycleMs = 10;
    g->nodeCount = 2;

    g->nodes[0] = (PlcNode){
            .id = 0,
            .type = PLC_NODE_CONST_BOOL,
            .valueType = PLC_VAL_BOOL,
            .inA = -1,
            .inB = -1,
            .paramInt = value ? 1 : 0
    };

    g->nodes[1] = (PlcNode){
            .id = 1,
            .type = PLC_NODE_DIGITAL_OUT,
            .valueType = PLC_VAL_BOOL,
            .inA = 0,
            .inB = -1,
            .paramInt = 0
    };
}

int main(void)
{
    plc_mem_init();

    uint32_t now = plc_port_now_ms();

    PlcGraph g;

    build_graph(&g, false);

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    now += 10;
    plc_tick(now);

    if (plc_port_test_get_do(0)) {
        printf("TEST FAILED: Graph A must set DO0=false\n");
        return 1;
    }

    build_graph(&g, true);

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    now += 10;
    plc_tick(now);

    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: Graph B must set DO0=true\n");
        return 1;
    }

    printf("TEST OK: graph hot swap\n");
    return 0;
}