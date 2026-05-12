#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "plc_link.h"

#ifdef __cplusplus
extern "C" {
#endif

bool plc_link_ext_handle(PlcLinkCommand cmd,
                         uint16_t seq,
                         const uint8_t* body,
                         uint16_t body_len);

#ifdef __cplusplus
}
#endif
