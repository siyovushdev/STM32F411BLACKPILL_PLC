#include "plc_persist.h"

#include "main.h"
#include "plc_graph_loader.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "friendly_plc/plc_safety.h"

#define PLC_PERSIST_MAGIC       0x46504C43UL /* 'FPLC' */
#define PLC_PERSIST_FORMAT_VER  1u
#define PLC_PERSIST_STATE_VALID 0xA5A55A5AUL
#define PLC_PERSIST_HDR_SIZE    64u

#if (PLC_PERSIST_SLOT_COUNT != 2u)
#error "This implementation expects exactly two A/B slots."
#endif

#if (PLC_PERSIST_SLOT_SIZE < (PLC_PERSIST_HDR_SIZE + PLC_PERSIST_MAX_IMAGE_SIZE))
#error "PLC_PERSIST_SLOT_SIZE is too small for PLC_PERSIST_MAX_IMAGE_SIZE."
#endif

typedef struct {
    uint32_t magic;
    uint16_t format_version;
    uint16_t header_size;
    uint32_t state;
    uint32_t sequence;
    uint32_t graph_version;
    uint32_t image_size;
    uint32_t image_crc32;
    uint32_t header_crc32;
    uint32_t reserved[7];
} PlcPersistHeader;

typedef struct {
    bool has_valid_image;
    uint8_t active_slot;
    uint32_t active_sequence;
    uint32_t active_version;
    uint32_t active_size;
    uint32_t active_crc32;
    PlcPersistResult last_result;
} PlcPersistContext;

static PlcPersistContext s_persist;

static uint32_t crc32_update(uint32_t crc, const uint8_t* data, uint32_t len)
{
    crc = ~crc;
    for (uint32_t i = 0u; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0u; bit < 8u; bit++) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1u) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

static uint32_t crc32_calc(const uint8_t* data, uint32_t len)
{
    return crc32_update(0u, data, len);
}

static uint32_t slot_base(uint8_t slot)
{
    return (slot == 0u) ? PLC_PERSIST_SLOT_A_BASE : PLC_PERSIST_SLOT_B_BASE;
}

static uint32_t slot_sector(uint8_t slot)
{
    return (slot == 0u) ? PLC_PERSIST_SLOT_A_SECTOR_ID : PLC_PERSIST_SLOT_B_SECTOR_ID;
}

static uint32_t header_crc32_calc(const PlcPersistHeader* hdr)
{
    PlcPersistHeader tmp = *hdr;
    tmp.header_crc32 = 0u;
    return crc32_calc((const uint8_t*)&tmp, sizeof(tmp));
}

static bool header_basic_valid(const PlcPersistHeader* hdr)
{
    if (hdr == NULL) return false;
    if (hdr->magic != PLC_PERSIST_MAGIC) return false;
    if (hdr->format_version != PLC_PERSIST_FORMAT_VER) return false;
    if (hdr->header_size != sizeof(PlcPersistHeader)) return false;
    if (hdr->state != PLC_PERSIST_STATE_VALID) return false;
    if (hdr->image_size == 0u || hdr->image_size > PLC_PERSIST_MAX_IMAGE_SIZE) return false;
    if ((PLC_PERSIST_HDR_SIZE + hdr->image_size) > PLC_PERSIST_SLOT_SIZE) return false;
    if (header_crc32_calc(hdr) != hdr->header_crc32) return false;
    return true;
}

static bool slot_valid(uint8_t slot, PlcPersistHeader* out_header)
{
    if (slot >= PLC_PERSIST_SLOT_COUNT) {
        return false;
    }

    const uint32_t base = slot_base(slot);
    const PlcPersistHeader* hdr = (const PlcPersistHeader*)base;

    if (!header_basic_valid(hdr)) {
        return false;
    }

    const uint8_t* image = (const uint8_t*)(base + PLC_PERSIST_HDR_SIZE);
    if (crc32_calc(image, hdr->image_size) != hdr->image_crc32) {
        return false;
    }

    if (out_header != NULL) {
        *out_header = *hdr;
    }
    return true;
}

static bool sequence_is_newer(uint32_t a, uint32_t b)
{
    return ((int32_t)(a - b)) > 0;
}

static bool find_best_slot(uint8_t* out_slot, PlcPersistHeader* out_header)
{
    bool found = false;
    uint8_t best_slot = 0u;
    PlcPersistHeader best_header;
    memset(&best_header, 0, sizeof(best_header));

    for (uint8_t slot = 0u; slot < PLC_PERSIST_SLOT_COUNT; slot++) {
        PlcPersistHeader hdr;
        if (!slot_valid(slot, &hdr)) {
            continue;
        }

        if (!found || sequence_is_newer(hdr.sequence, best_header.sequence)) {
            found = true;
            best_slot = slot;
            best_header = hdr;
        }
    }

    if (!found) {
        return false;
    }

    if (out_slot != NULL) *out_slot = best_slot;
    if (out_header != NULL) *out_header = best_header;
    return true;
}

static void refresh_status(void)
{
    uint8_t slot = 0u;
    PlcPersistHeader hdr;
    if (find_best_slot(&slot, &hdr)) {
        s_persist.has_valid_image = true;
        s_persist.active_slot = slot;
        s_persist.active_sequence = hdr.sequence;
        s_persist.active_version = hdr.graph_version;
        s_persist.active_size = hdr.image_size;
        s_persist.active_crc32 = hdr.image_crc32;
    } else {
        s_persist.has_valid_image = false;
        s_persist.active_slot = 0xFFu;
        s_persist.active_sequence = 0u;
        s_persist.active_version = 0u;
        s_persist.active_size = 0u;
        s_persist.active_crc32 = 0u;
    }
}

static void flash_clear_flags(void)
{
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
}

static PlcPersistResult flash_erase_slot(uint8_t slot)
{
    if (slot >= PLC_PERSIST_SLOT_COUNT) {
        return PLC_PERSIST_ERR_ARG;
    }

    HAL_FLASH_Unlock();
    flash_clear_flags();

    FLASH_EraseInitTypeDef erase;
    memset(&erase, 0, sizeof(erase));
    erase.TypeErase = TYPEERASE_SECTORS;
    erase.Sector = slot_sector(slot);
    erase.NbSectors = 1u;
    erase.VoltageRange = VOLTAGE_RANGE_3;

    uint32_t sector_error = 0u;
    HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&erase, &sector_error);

    HAL_FLASH_Lock();

    if (st != HAL_OK || sector_error != 0xFFFFFFFFu) {
        return PLC_PERSIST_ERR_ERASE;
    }
    return PLC_PERSIST_OK;
}

static PlcPersistResult flash_program(uint32_t addr, const uint8_t* data, uint32_t len)
{
    if ((addr & 0x3u) != 0u || data == NULL) {
        return PLC_PERSIST_ERR_ARG;
    }

    HAL_FLASH_Unlock();
    flash_clear_flags();

    const uint32_t aligned_len = (len + 3u) & ~3u;
    for (uint32_t off = 0u; off < aligned_len; off += 4u) {
        uint32_t word = 0xFFFFFFFFu;
        for (uint32_t b = 0u; b < 4u; b++) {
            const uint32_t k = off + b;
            ((uint8_t*)&word)[b] = (k < len) ? data[k] : 0xFFu;
        }

        HAL_StatusTypeDef st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + off, word);
        if (st != HAL_OK) {
            HAL_FLASH_Lock();
            return PLC_PERSIST_ERR_WRITE;
        }

        if (*(volatile uint32_t*)(addr + off) != word) {
            HAL_FLASH_Lock();
            return PLC_PERSIST_ERR_VERIFY;
        }
    }

    HAL_FLASH_Lock();
    return PLC_PERSIST_OK;
}

void plc_persist_init(void)
{
    memset(&s_persist, 0, sizeof(s_persist));
    s_persist.active_slot = 0xFFu;
    s_persist.last_result = PLC_PERSIST_OK;
    refresh_status();
}

PlcPersistResult plc_persist_save_image(const uint8_t* image, uint32_t size, uint32_t version)
{
    if (image == NULL) {
        s_persist.last_result = PLC_PERSIST_ERR_ARG;
        return s_persist.last_result;
    }
    if (size == 0u || size > PLC_PERSIST_MAX_IMAGE_SIZE) {
        s_persist.last_result = PLC_PERSIST_ERR_SIZE;
        return s_persist.last_result;
    }

    refresh_status();

    const uint8_t target_slot = s_persist.has_valid_image ? (uint8_t)(s_persist.active_slot ^ 1u) : 0u;
    const uint32_t sequence = s_persist.has_valid_image ? (s_persist.active_sequence + 1u) : 1u;
    const uint32_t base = slot_base(target_slot);

    PlcPersistHeader hdr;
    memset(&hdr, 0xFF, sizeof(hdr));
    hdr.magic = PLC_PERSIST_MAGIC;
    hdr.format_version = PLC_PERSIST_FORMAT_VER;
    hdr.header_size = sizeof(PlcPersistHeader);
    hdr.state = PLC_PERSIST_STATE_VALID;
    hdr.sequence = sequence;
    hdr.graph_version = version;
    hdr.image_size = size;
    hdr.image_crc32 = crc32_calc(image, size);
    hdr.header_crc32 = 0u;
    hdr.header_crc32 = header_crc32_calc(&hdr);

    PlcPersistResult r = flash_erase_slot(target_slot);
    if (r != PLC_PERSIST_OK) {
        s_persist.last_result = r;
        return r;
    }

    r = flash_program(base + PLC_PERSIST_HDR_SIZE, image, size);
    if (r != PLC_PERSIST_OK) {
        s_persist.last_result = r;
        return r;
    }

    r = flash_program(base, (const uint8_t*)&hdr, sizeof(hdr));
    if (r != PLC_PERSIST_OK) {
        s_persist.last_result = r;
        return r;
    }

    if (!slot_valid(target_slot, NULL)) {
        s_persist.last_result = PLC_PERSIST_ERR_VERIFY;

        plc_fault_note_persist_corrupt(PLC_PERSIST_ERR_VERIFY);

        return s_persist.last_result;
    }

    refresh_status();
    s_persist.last_result = PLC_PERSIST_OK;
    return PLC_PERSIST_OK;
}

PlcPersistResult plc_persist_load_active(void)
{
    refresh_status();
    if (!s_persist.has_valid_image) {
        s_persist.last_result = PLC_PERSIST_ERR_EMPTY;
        return s_persist.last_result;
    }

    const uint32_t base = slot_base(s_persist.active_slot);
    const uint8_t* image = (const uint8_t*)(base + PLC_PERSIST_HDR_SIZE);

    if (!plc_graph_loader_apply_image(image, s_persist.active_size, s_persist.active_version)) {
        s_persist.last_result = PLC_PERSIST_ERR_APPLY;

        plc_fault_note_persist_corrupt(PLC_PERSIST_ERR_APPLY);

        return s_persist.last_result;
    }

    s_persist.last_result = PLC_PERSIST_OK;
    return PLC_PERSIST_OK;
}

void plc_persist_get_status(PlcPersistStatus* out_status)
{
    if (out_status == NULL) {
        return;
    }

    refresh_status();
    out_status->has_valid_image = s_persist.has_valid_image;
    out_status->active_slot = s_persist.active_slot;
    out_status->active_sequence = s_persist.active_sequence;
    out_status->active_version = s_persist.active_version;
    out_status->active_size = s_persist.active_size;
    out_status->active_crc32 = s_persist.active_crc32;
    out_status->last_result = s_persist.last_result;
}

const char* plc_persist_result_name(PlcPersistResult result)
{
    switch (result) {
        case PLC_PERSIST_OK: return "OK";
        case PLC_PERSIST_ERR_ARG: return "ARG";
        case PLC_PERSIST_ERR_EMPTY: return "EMPTY";
        case PLC_PERSIST_ERR_SIZE: return "SIZE";
        case PLC_PERSIST_ERR_HEADER: return "HEADER";
        case PLC_PERSIST_ERR_CRC: return "CRC";
        case PLC_PERSIST_ERR_ERASE: return "ERASE";
        case PLC_PERSIST_ERR_WRITE: return "WRITE";
        case PLC_PERSIST_ERR_VERIFY: return "VERIFY";
        case PLC_PERSIST_ERR_APPLY: return "APPLY";
        default: return "UNKNOWN";
    }
}
