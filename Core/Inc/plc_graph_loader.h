#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PLC_GRAPH_LOADER_MAX_IMAGE_SIZE
#define PLC_GRAPH_LOADER_MAX_IMAGE_SIZE 16384u
#endif

typedef enum {
    PLC_GRAPH_LOADER_OK = 0,
    PLC_GRAPH_LOADER_ERR_BUSY,
    PLC_GRAPH_LOADER_ERR_NOT_STARTED,
    PLC_GRAPH_LOADER_ERR_SIZE,
    PLC_GRAPH_LOADER_ERR_OFFSET,
    PLC_GRAPH_LOADER_ERR_CRC,
    PLC_GRAPH_LOADER_ERR_VERSION,
    PLC_GRAPH_LOADER_ERR_APPLY_FAILED
} PlcGraphLoaderResult;

typedef struct {
    uint32_t active_version;
    uint32_t upload_size;
    uint32_t uploaded_bytes;
    uint32_t expected_crc32;
    uint32_t last_error;
    bool upload_active;
    bool image_ready;
} PlcGraphLoaderStatus;

void plc_graph_loader_init(void);
PlcGraphLoaderResult plc_graph_loader_begin(uint32_t graph_version, uint32_t total_size, uint32_t crc32);
PlcGraphLoaderResult plc_graph_loader_chunk(uint32_t offset, const uint8_t* data, uint16_t len);
PlcGraphLoaderResult plc_graph_loader_end(void);
PlcGraphLoaderResult plc_graph_loader_activate(void);
void plc_graph_loader_get_status(PlcGraphLoaderStatus* out_status);

const uint8_t* plc_graph_loader_get_pending_image(void);
uint32_t plc_graph_loader_get_pending_image_size(void);

const uint8_t* plc_graph_loader_get_active_image(void);
uint32_t plc_graph_loader_get_active_image_size(void);
uint32_t plc_graph_loader_get_active_image_crc32(void);

bool plc_graph_loader_apply_image(const uint8_t* image, uint32_t size, uint32_t version);

void plc_graph_loader_cancel(void);

#ifdef __cplusplus
}
#endif
