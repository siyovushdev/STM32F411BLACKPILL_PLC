#include "friendly_plc/plc.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    plc_mem_init();

    plc_mem_set_int(0, 1234);

    int32_t v = plc_mem_get_int(0);

    if (v != 1234) {
        printf("TEST FAILED: MEM_INT mismatch\n");
        return 1;
    }

    printf("TEST OK: MEM_INT\n");
    return 0;
}
