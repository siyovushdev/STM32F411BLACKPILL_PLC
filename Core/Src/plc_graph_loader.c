#include "plc_graph_loader.h"
#include "friendly_plc/plc.h"
#include "friendly_plc/plc_types.h"
#include "friendly_plc/plc_safety.h"
#include "friendly_plc/plc_log.h"
#include <string.h>


typedef struct {
    uint8_t image[PLC_GRAPH_LOADER_MAX_IMAGE_SIZE];
    uint8_t written_map[(PLC_GRAPH_LOADER_MAX_IMAGE_SIZE + 7u) / 8u];

    uint32_t pending_version;
    uint32_t active_version;
    uint32_t active_size;
    uint32_t active_crc32;

    uint32_t total_size;
    uint32_t uploaded_bytes;
    uint32_t expected_crc32;
    uint32_t last_error;
    bool upload_active;
    bool image_ready;
} PlcGraphLoaderContext;

static PlcGraphLoaderContext s_loader;

static uint32_t crc32_update(uint32_t crc, const uint8_t* data, uint32_t len)
{
    crc = ~crc;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8u; bit++) {
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

static void mark_range_written(uint32_t offset, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        uint32_t pos = offset + i;
        s_loader.written_map[pos / 8u] |= (uint8_t)(1u << (pos % 8u));
    }
}

static bool is_range_unwritten(uint32_t offset, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        uint32_t pos = offset + i;
        if ((s_loader.written_map[pos / 8u] & (uint8_t)(1u << (pos % 8u))) != 0u) {
            return false;
        }
    }
    return true;
}

static bool is_all_written(uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        if ((s_loader.written_map[i / 8u] & (uint8_t)(1u << (i % 8u))) == 0u) {
            return false;
        }
    }
    return true;
}

static void log_graph_param_diag(const char* stage, const PlcGraph* graph)
{
#if PLC_LOG_ENABLED
    if (graph == NULL) {
        PLC_LOGT(PLC_LOG_TAG, "%s: graph=NULL", stage);
        return;
    }

    PLC_LOGT(PLC_LOG_TAG,
             "%s: graph=%p cycleMs=%lu nodeCount=%u sizeof(PlcGraph)=%u sizeof(PlcNode)=%u",
             stage,
             (const void*)graph,
             (unsigned long)graph->cycleMs,
             (unsigned)graph->nodeCount,
             (unsigned)sizeof(PlcGraph),
             (unsigned)sizeof(PlcNode));

    const uint16_t count = graph->nodeCount < 8u ? graph->nodeCount : 8u;
    for (uint16_t i = 0u; i < count; i++) {
        const PlcNode* n = &graph->nodes[i];
        PLC_LOGT(PLC_LOG_TAG,
                 "%s: node[%u] id=%u type=%u inA=%d inB=%d paramInt=%ld paramMs=%lu flags=0x%08lX",
                 stage,
                 (unsigned)i,
                 (unsigned)n->id,
                 (unsigned)n->type,
                 (int)n->inA,
                 (int)n->inB,
                 (long)n->paramInt,
                 (unsigned long)n->paramMs,
                 (unsigned long)n->flags);
    }
#endif
}

void plc_graph_loader_init(void)
{
    memset(&s_loader, 0, sizeof(s_loader));
}

PlcGraphLoaderResult plc_graph_loader_begin(uint32_t graph_version, uint32_t total_size, uint32_t crc32)
{
    if (s_loader.upload_active) {
        s_loader.last_error = PLC_GRAPH_LOADER_ERR_BUSY;
        return PLC_GRAPH_LOADER_ERR_BUSY;
    }
    if (total_size == 0u || total_size > PLC_GRAPH_LOADER_MAX_IMAGE_SIZE) {
        s_loader.last_error = PLC_GRAPH_LOADER_ERR_SIZE;

        plc_fault_report(
                PLC_FAULT_DOMAIN_GRAPH,
                PLC_FAULT_INVALID_GRAPH,
                PLC_FAULT_SEVERITY_WARNING,
                (int32_t)total_size
        );

        return PLC_GRAPH_LOADER_ERR_SIZE;
    }

    memset(s_loader.image, 0, sizeof(s_loader.image));
    memset(s_loader.written_map, 0, sizeof(s_loader.written_map));

    s_loader.pending_version = graph_version;
    s_loader.total_size = total_size;
    s_loader.uploaded_bytes = 0u;
    s_loader.expected_crc32 = crc32;
    s_loader.upload_active = true;
    s_loader.image_ready = false;
    s_loader.last_error = PLC_GRAPH_LOADER_OK;

    return PLC_GRAPH_LOADER_OK;
}

PlcGraphLoaderResult plc_graph_loader_chunk(uint32_t offset, const uint8_t* data, uint16_t len)
{
    if (!s_loader.upload_active) {
        s_loader.last_error = PLC_GRAPH_LOADER_ERR_NOT_STARTED;
        return PLC_GRAPH_LOADER_ERR_NOT_STARTED;
    }
    if (data == NULL || len == 0u) {
        s_loader.last_error = PLC_GRAPH_LOADER_ERR_SIZE;
        return PLC_GRAPH_LOADER_ERR_SIZE;
    }
    if (offset > s_loader.total_size || ((uint32_t)len > (s_loader.total_size - offset))) {
        s_loader.last_error = PLC_GRAPH_LOADER_ERR_OFFSET;
        return PLC_GRAPH_LOADER_ERR_OFFSET;
    }
    if (!is_range_unwritten(offset, len)) {
        s_loader.last_error = PLC_GRAPH_LOADER_ERR_OFFSET;
        return PLC_GRAPH_LOADER_ERR_OFFSET;
    }

    memcpy(&s_loader.image[offset], data, len);
    mark_range_written(offset, len);
    s_loader.uploaded_bytes += len;
    s_loader.last_error = PLC_GRAPH_LOADER_OK;

    return PLC_GRAPH_LOADER_OK;
}

PlcGraphLoaderResult plc_graph_loader_end(void)
{
    if (!s_loader.upload_active) {
        s_loader.last_error = PLC_GRAPH_LOADER_ERR_NOT_STARTED;
        return PLC_GRAPH_LOADER_ERR_NOT_STARTED;
    }
    if (s_loader.uploaded_bytes != s_loader.total_size || !is_all_written(s_loader.total_size)) {
        s_loader.last_error = PLC_GRAPH_LOADER_ERR_SIZE;

        plc_fault_report(
                PLC_FAULT_DOMAIN_GRAPH,
                PLC_FAULT_INVALID_GRAPH,
                PLC_FAULT_SEVERITY_WARNING,
                (int32_t)s_loader.uploaded_bytes
        );

        return PLC_GRAPH_LOADER_ERR_SIZE;
    }

    PLC_LOGI("GRAPH_LOADER",
             "upload_end: uploaded=%lu total=%lu expected_crc=0x%08lX",
             (unsigned long)s_loader.uploaded_bytes,
             (unsigned long)s_loader.total_size,
             (unsigned long)s_loader.expected_crc32);

    uint32_t crc = crc32_calc(s_loader.image, s_loader.total_size);

    PLC_LOGI("GRAPH_LOADER",
             "upload_end: calculated_crc=0x%08lX",
             (unsigned long)crc);

    if (crc != s_loader.expected_crc32) {
        PLC_LOGE("GRAPH_LOADER",
                 "upload_end failed: CRC mismatch calc=0x%08lX expected=0x%08lX",
                 (unsigned long)crc,
                 (unsigned long)s_loader.expected_crc32);

        s_loader.upload_active = false;
        s_loader.image_ready = false;
        s_loader.last_error = PLC_GRAPH_LOADER_ERR_CRC;

        plc_enter_safe(
                PLC_FAULT_DOMAIN_GRAPH,
                PLC_FAULT_INVALID_GRAPH,
                (int32_t)crc
        );

        return PLC_GRAPH_LOADER_ERR_CRC;
    }

    s_loader.upload_active = false;
    s_loader.image_ready = true;
    s_loader.last_error = PLC_GRAPH_LOADER_OK;

    PLC_LOGI("GRAPH_LOADER", "upload_end OK: image_ready=1");

    return PLC_GRAPH_LOADER_OK;
}

PlcGraphLoaderResult plc_graph_loader_activate(void)
{
    if (!s_loader.image_ready) {
        s_loader.last_error = PLC_GRAPH_LOADER_ERR_NOT_STARTED;
        return PLC_GRAPH_LOADER_ERR_NOT_STARTED;
    }

    if (!plc_graph_loader_apply_image(s_loader.image, s_loader.total_size, s_loader.pending_version)) {
        s_loader.last_error = PLC_GRAPH_LOADER_ERR_APPLY_FAILED;

        plc_enter_fault(
                PLC_FAULT_DOMAIN_GRAPH,
                PLC_FAULT_INVALID_GRAPH,
                PLC_GRAPH_LOADER_ERR_APPLY_FAILED
        );

        return PLC_GRAPH_LOADER_ERR_APPLY_FAILED;
    }

    s_loader.active_version = s_loader.pending_version;
    s_loader.active_size = s_loader.total_size;
    s_loader.active_crc32 = s_loader.expected_crc32;
    s_loader.image_ready = false;
    s_loader.last_error = PLC_GRAPH_LOADER_OK;

    return PLC_GRAPH_LOADER_OK;
}

void plc_graph_loader_get_status(PlcGraphLoaderStatus* out_status)
{
    if (out_status == NULL) {
        return;
    }

    out_status->active_version = s_loader.active_version;
    out_status->upload_size = s_loader.total_size;
    out_status->uploaded_bytes = s_loader.uploaded_bytes;
    out_status->expected_crc32 = s_loader.expected_crc32;
    out_status->last_error = s_loader.last_error;
    out_status->upload_active = s_loader.upload_active;
    out_status->image_ready = s_loader.image_ready;
}

bool plc_graph_loader_apply_image(const uint8_t* image, uint32_t size, uint32_t version)
{
    PLC_LOGI("GRAPH_LOADER",
             "apply_image start: image=%p size=%lu expected=%u version=%lu",
             image,
             (unsigned long)size,
             (unsigned)sizeof(PlcGraph),
             (unsigned long)version);

    if (image == NULL) {
        PLC_LOGE("GRAPH_LOADER", "apply_image failed: image=NULL");
        return false;
    }

    if (size != sizeof(PlcGraph)) {
        PLC_LOGE("GRAPH_LOADER",
                 "apply_image failed: bad size=%lu expected=%u",
                 (unsigned long)size,
                 (unsigned)sizeof(PlcGraph));
        return false;
    }

    const PlcGraph* graph = (const PlcGraph*)image;

    PLC_LOGI("GRAPH_LOADER",
             "graph header: nodeCount=%u cycleMs=%lu",
             (unsigned)graph->nodeCount,
             (unsigned long)graph->cycleMs);

    if (!plc_validate_graph(graph)) {
        PLC_LOGE("GRAPH_LOADER", "apply_image failed: plc_validate_graph()");
        return false;
    }

    PLC_LOGI("GRAPH_LOADER", "validate OK");

    if (!plc_upload_graph(graph)) {
        PLC_LOGE("GRAPH_LOADER", "apply_image failed: plc_upload_graph()");
        return false;
    }

    PLC_LOGI("GRAPH_LOADER", "upload_graph OK");

    if (!plc_request_activate_graph()) {
        PLC_LOGE("GRAPH_LOADER", "apply_image failed: plc_request_activate_graph()");
        return false;
    }

    PLC_LOGI("GRAPH_LOADER", "request_activate_graph OK");

    return true;
}

const uint8_t* plc_graph_loader_get_pending_image(void)
{
    if (!s_loader.image_ready) {
        return NULL;
    }

    return s_loader.image;
}

uint32_t plc_graph_loader_get_pending_image_size(void)
{
    if (!s_loader.image_ready) {
        return 0u;
    }

    return s_loader.total_size;
}

const uint8_t* plc_graph_loader_get_active_image(void)
{
    if (s_loader.active_size == 0u) {
        return NULL;
    }

    return s_loader.image;
}

uint32_t plc_graph_loader_get_active_image_size(void)
{
    return s_loader.active_size;
}

uint32_t plc_graph_loader_get_active_image_crc32(void)
{
    return s_loader.active_crc32;
}

void plc_graph_loader_cancel(void)
{
    s_loader.upload_active = false;
    s_loader.image_ready = false;
    s_loader.uploaded_bytes = 0u;
    s_loader.total_size = 0u;
    s_loader.expected_crc32 = 0u;
    memset(s_loader.image, 0, sizeof(s_loader.image));
    memset(s_loader.written_map, 0, sizeof(s_loader.written_map));
}
