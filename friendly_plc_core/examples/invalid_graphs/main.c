#include "friendly_plc/plc.h"
#include "friendly_plc/plc_error.h"

#include <stdio.h>
#include <string.h>

static int check_bad_ref(void)
{
    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 2;

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_CONST_BOOL, .valueType=PLC_VAL_BOOL, .inA=-1, .inB=-1, .paramInt=1 };
    g.nodes[1] = (PlcNode){ .id=1, .type=PLC_NODE_NOT, .valueType=PLC_VAL_BOOL, .inA=1, .inB=-1 };

    PlcError err = plc_validate_graph_ex(&g);

    if (err != PLC_ERR_BAD_INPUT_REF) {
        printf("TEST FAILED: expected BAD_INPUT_REF, got %d (%s)\n", (int)err, plc_error_to_string(err));
        return 1;
    }

    return 0;
}

static int check_bad_channel(void)
{
    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 1;

    g.nodes[0] = (PlcNode){ .id=0, .type=PLC_NODE_DIGITAL_IN, .valueType=PLC_VAL_BOOL, .inA=-1, .inB=-1, .paramInt=999 };

    PlcError err = plc_validate_graph_ex(&g);

    if (err != PLC_ERR_BAD_HW_CHANNEL) {
        printf("TEST FAILED: expected BAD_HW_CHANNEL, got %d (%s)\n", (int)err, plc_error_to_string(err));
        return 1;
    }

    return 0;
}

int main(void)
{
    if (check_bad_ref() != 0) return 1;
    if (check_bad_channel() != 0) return 1;

    printf("TEST OK: invalid graph cases\n");
    return 0;
}
