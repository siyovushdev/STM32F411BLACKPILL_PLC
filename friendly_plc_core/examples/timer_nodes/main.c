#include "friendly_plc/plc.h"
#include "friendly_plc/plc_port.h"

#include <stdio.h>
#include <string.h>

static void tick_many(uint32_t* now, int count)
{
    for (int i = 0; i < count; i++) {
        *now += 10;
        plc_tick(*now);
    }
}

static int test_toff(void)
{
    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 3;

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_DIGITAL_IN, .valueType=PLC_VAL_BOOL, .inA=-1, .inB=-1, .paramInt=0 };
    g.nodes[1] = (PlcNode){ .id=1, .type=PLC_NODE_TOFF, .valueType=PLC_VAL_BOOL, .inA=0, .inB=-1, .paramMs=50 };
    g.nodes[2] = (PlcNode){ .id=2, .type=PLC_NODE_DIGITAL_OUT, .valueType=PLC_VAL_BOOL, .inA=1, .inB=-1, .paramInt=0 };

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    uint32_t now = plc_port_now_ms();

    plc_port_test_set_di(0, true);
    tick_many(&now, 4);

    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: TOFF output must be true while input true\n");
        return 1;
    }

    plc_port_test_set_di(0, false);
    tick_many(&now, 3);

    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: TOFF output must hold true before timeout\n");
        return 1;
    }

    tick_many(&now, 5);

    if (plc_port_test_get_do(0)) {
        printf("TEST FAILED: TOFF output must be false after timeout\n");
        return 1;
    }

    return 0;
}

static int test_tp(void)
{
    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 3;

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_DIGITAL_IN, .valueType=PLC_VAL_BOOL, .inA=-1, .inB=-1, .paramInt=0 };
    g.nodes[1] = (PlcNode){ .id=1, .type=PLC_NODE_TP, .valueType=PLC_VAL_BOOL, .inA=0, .inB=-1, .paramMs=50 };
    g.nodes[2] = (PlcNode){ .id=2, .type=PLC_NODE_DIGITAL_OUT, .valueType=PLC_VAL_BOOL, .inA=1, .inB=-1, .paramInt=0 };

    if (!plc_upload_graph(&g)) return 1;
    if (!plc_request_activate_graph()) return 1;

    uint32_t now = plc_port_now_ms();

    plc_port_test_set_di(0, false);
    tick_many(&now, 3);

    plc_port_test_set_di(0, true);
    tick_many(&now, 4);

    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: TP output must start on rising edge\n");
        return 1;
    }

    plc_port_test_set_di(0, false);
    tick_many(&now, 3);

    if (!plc_port_test_get_do(0)) {
        printf("TEST FAILED: TP output must remain true before pulse timeout\n");
        return 1;
    }

    tick_many(&now, 5);

    if (plc_port_test_get_do(0)) {
        printf("TEST FAILED: TP output must be false after pulse timeout\n");
        return 1;
    }

    return 0;
}

int main(void)
{
    plc_mem_init();

    if (test_toff() != 0) return 1;
    if (test_tp() != 0) return 1;

    printf("TEST OK: TOFF and TP\n");
    return 0;
}
