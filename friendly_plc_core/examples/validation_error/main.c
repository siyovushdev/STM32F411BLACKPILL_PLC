#include "friendly_plc/plc.h"
#include "friendly_plc/plc_error.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    PlcGraph g;
    memset(&g, 0, sizeof(g));

    g.cycleMs = 10;
    g.nodeCount = 1;

    g.nodes[0].id = 5; // ошибка: должен быть 0
    g.nodes[0].type = PLC_NODE_CONST_BOOL;
    g.nodes[0].valueType = PLC_VAL_BOOL;
    g.nodes[0].inA = -1;
    g.nodes[0].inB = -1;
    g.nodes[0].paramInt = 1;

    PlcError err = plc_validate_graph_ex(&g);

    if (err != PLC_ERR_BAD_NODE_ID) {
        printf("TEST FAILED: expected PLC_ERR_BAD_NODE_ID, got %d (%s)\n",
               (int)err,
               plc_error_to_string(err));
        return 1;
    }

    printf("TEST OK: validation error = %s\n", plc_error_to_string(err));
    return 0;
}