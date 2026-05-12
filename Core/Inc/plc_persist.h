#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * STM32F411CEU6 512 KB Flash production layout for graph persistence:
 *
 *   0x08000000 - 0x0803FFFF  Firmware, 256 KB
 *   0x08040000 - 0x0805FFFF  Persist Slot A, Sector 6, 128 KB
 *   0x08060000 - 0x0807FFFF  Persist Slot B, Sector 7, 128 KB
 *
 * Linker script must reserve Sector 6/7 from firmware:
 *   FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 256K
 */
#ifndef PLC_PERSIST_SLOT_A_SECTOR_ID
#define PLC_PERSIST_SLOT_A_SECTOR_ID   FLASH_SECTOR_6
#endif

#ifndef PLC_PERSIST_SLOT_B_SECTOR_ID
#define PLC_PERSIST_SLOT_B_SECTOR_ID   FLASH_SECTOR_7
#endif

#ifndef PLC_PERSIST_SLOT_A_BASE
#define PLC_PERSIST_SLOT_A_BASE        0x08040000UL
#endif

#ifndef PLC_PERSIST_SLOT_B_BASE
#define PLC_PERSIST_SLOT_B_BASE        0x08060000UL
#endif

#ifndef PLC_PERSIST_SLOT_SIZE
#define PLC_PERSIST_SLOT_SIZE          (128UL * 1024UL)
#endif

#ifndef PLC_PERSIST_SLOT_COUNT
#define PLC_PERSIST_SLOT_COUNT         2u
#endif

#ifndef PLC_PERSIST_MAX_IMAGE_SIZE
#define PLC_PERSIST_MAX_IMAGE_SIZE     16384u
#endif

typedef enum {
    PLC_PERSIST_OK = 0,
    PLC_PERSIST_ERR_ARG,
    PLC_PERSIST_ERR_EMPTY,
    PLC_PERSIST_ERR_SIZE,
    PLC_PERSIST_ERR_HEADER,
    PLC_PERSIST_ERR_CRC,
    PLC_PERSIST_ERR_ERASE,
    PLC_PERSIST_ERR_WRITE,
    PLC_PERSIST_ERR_VERIFY,
    PLC_PERSIST_ERR_APPLY
} PlcPersistResult;

typedef struct {
    bool has_valid_image;
    uint8_t active_slot;
    uint32_t active_sequence;
    uint32_t active_version;
    uint32_t active_size;
    uint32_t active_crc32;
    PlcPersistResult last_result;
} PlcPersistStatus;

void plc_persist_init(void);
PlcPersistResult plc_persist_save_image(const uint8_t* image, uint32_t size, uint32_t version);
PlcPersistResult plc_persist_load_active(void);
void plc_persist_get_status(PlcPersistStatus* out_status);
const char* plc_persist_result_name(PlcPersistResult result);

#ifdef __cplusplus
}
#endif
