#include "plc_link.h"

#include "plc_link_ext.h"
#include "plc_diag.h"
#include "plc_graph_loader.h"
#include "plc_link_uart.h"
#include "plc_persist.h"
#include "plc_persist_service.h"
#include "plc_platform.h"

#include "friendly_plc/plc_safety.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define HDR_SIZE 4u
#define U16_LE(p) ((uint16_t)((uint16_t)(p)[0] | ((uint16_t)(p)[1] << 8u)))
#define U32_LE(p) ((uint32_t)((uint32_t)(p)[0] | ((uint32_t)(p)[1] << 8u) | ((uint32_t)(p)[2] << 16u) | ((uint32_t)(p)[3] << 24u)))

typedef struct {
    PlcLinkStatus status;
} PlcLinkContext;

static PlcLinkContext s_link;

static void put_u16(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8u) & 0xFFu);
}

static void put_u32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8u) & 0xFFu);
    p[2] = (uint8_t)((v >> 16u) & 0xFFu);
    p[3] = (uint8_t)((v >> 24u) & 0xFFu);
}

static bool send_packet(uint8_t cmd, uint16_t seq, const uint8_t* body, uint16_t body_len)
{
    if (body_len > PLC_LINK_MAX_BODY_SIZE) {
        return false;
    }

    uint8_t payload[HDR_SIZE + PLC_LINK_MAX_BODY_SIZE];
    payload[0] = PLC_LINK_PROTO_VERSION;
    payload[1] = cmd;
    put_u16(&payload[2], seq);

    if (body_len > 0u && body != NULL) {
        memcpy(&payload[HDR_SIZE], body, body_len);
    }

    PlcLinkUartResult r = plc_link_uart_send(payload, (uint16_t)(HDR_SIZE + body_len), 100u);
    if (r == PLC_LINK_UART_OK) {
        s_link.status.tx_frames++;
        return true;
    }

    s_link.status.errors++;
    s_link.status.last_error = PLC_LINK_ERR_TX;
    return false;
}

static bool send_ack(uint16_t seq, uint8_t ack_for)
{
    uint8_t body[1] = { ack_for };
    return send_packet(PLC_LINK_RSP_ACK, seq, body, sizeof(body));
}

static bool send_error(uint16_t seq, uint8_t code, uint32_t detail)
{
    uint8_t body[5];
    body[0] = code;
    put_u32(&body[1], detail);
    s_link.status.errors++;
    s_link.status.last_error = code;

    if (code == PLC_LINK_ERR_BAD_SIZE ||
        code == PLC_LINK_ERR_BAD_VERSION ||
        code == PLC_LINK_ERR_UNKNOWN_CMD) {
        plc_fault_note_protocol_error(
                PLC_FAULT_PROTOCOL_FRAME,
                ((int32_t)code << 16) | (int32_t)(detail & 0xFFFFu)
        );
    }

    return send_packet(PLC_LINK_RSP_ERROR, seq, body, sizeof(body));
}

bool plc_link_send_response(uint8_t rsp_cmd, uint16_t seq, const uint8_t* body, uint16_t body_len)
{
    return send_packet(rsp_cmd, seq, body, body_len);
}

bool plc_link_send_error_response(uint16_t seq, uint8_t error_code, uint32_t detail)
{
    return send_error(seq, error_code, detail);
}

static bool send_status(uint16_t seq)
{
    PlcGraphLoaderStatus gs;
    PlcLinkUartStats us;
    PlcPersistStatus ps;
    PlcPersistServiceStatus pss;
    PlcDiagStatus ds;

    plc_graph_loader_get_status(&gs);
    plc_link_uart_get_stats(&us);
    plc_persist_get_status(&ps);
    plc_persist_service_get_status(&pss);
    plc_diag_get_status(&ds);

    uint8_t body[160];
    memset(body, 0, sizeof(body));

    uint32_t graph_flags = 0u;
    if (gs.upload_active) { graph_flags |= 0x01u; }
    if (gs.image_ready) { graph_flags |= 0x02u; }

    uint32_t persist_flags = 0u;
    if (ps.has_valid_image) { persist_flags |= 0x01u; }
    if (pss.initialized) { persist_flags |= 0x02u; }
    if (pss.busy) { persist_flags |= 0x04u; }
    if (pss.pending) { persist_flags |= 0x08u; }
    if (pss.copying) { persist_flags |= 0x10u; }

    put_u32(&body[0], plc_platform_now_ms());
    put_u32(&body[4], gs.active_version);
    put_u32(&body[8], gs.upload_size);
    put_u32(&body[12], gs.uploaded_bytes);
    put_u32(&body[16], graph_flags);
    put_u32(&body[20], gs.last_error);
    put_u32(&body[24], us.rx_frames_ok);
    put_u32(&body[28], us.rx_crc_errors);
    put_u32(&body[32], us.rx_overflows);
    put_u32(&body[36], us.tx_frames);
    put_u32(&body[40], us.tx_errors);

    put_u32(&body[44], persist_flags);
    put_u32(&body[48], (uint32_t)ps.last_result);
    put_u32(&body[52], (uint32_t)ps.active_slot);
    put_u32(&body[56], ps.active_sequence);
    put_u32(&body[60], ps.active_version);
    put_u32(&body[64], ps.active_size);
    put_u32(&body[68], ps.active_crc32);

    put_u32(&body[72], pss.request_count);
    put_u32(&body[76], pss.success_count);
    put_u32(&body[80], pss.fail_count);
    put_u32(&body[84], pss.rejected_count);
    put_u32(&body[88], pss.last_duration_ms);
    put_u32(&body[92], (uint32_t)pss.last_result);

    put_u32(&body[96], ds.heap_total_bytes);
    put_u32(&body[100], ds.heap_free_bytes);
    put_u32(&body[104], ds.heap_min_ever_free_bytes);
    put_u32(&body[108], ds.task_registered_flags);

    put_u32(&body[112], ds.scan_stack_free_words);
    put_u32(&body[116], ds.scan_stack_used_words);
    put_u32(&body[120], ds.scan_stack_size_words);

    put_u32(&body[124], ds.link_rx_stack_free_words);
    put_u32(&body[128], ds.link_rx_stack_used_words);
    put_u32(&body[132], ds.link_rx_stack_size_words);

    put_u32(&body[136], ds.link_tx_stack_free_words);
    put_u32(&body[140], ds.link_tx_stack_used_words);
    put_u32(&body[144], ds.link_tx_stack_size_words);

    put_u32(&body[148], ds.persist_stack_free_words);
    put_u32(&body[152], ds.persist_stack_used_words);
    put_u32(&body[156], ds.persist_stack_size_words);

    return send_packet(PLC_LINK_RSP_STATUS, seq, body, sizeof(body));
}

void plc_link_init(void)
{
    memset(&s_link, 0, sizeof(s_link));
}

void plc_link_on_frame(const uint8_t* payload, uint16_t payload_len, void* user)
{
    (void)user;

    if (payload == NULL || payload_len < HDR_SIZE) {
        (void)send_error(0u, PLC_LINK_ERR_BAD_SIZE, payload_len);
        return;
    }

    uint8_t version = payload[0];
    uint8_t cmd = payload[1];
    uint16_t seq = U16_LE(&payload[2]);
    const uint8_t* body = &payload[HDR_SIZE];
    uint16_t body_len = (uint16_t)(payload_len - HDR_SIZE);

    s_link.status.rx_frames++;
    s_link.status.last_seq = seq;

    if (version != PLC_LINK_PROTO_VERSION) {
        (void)send_error(seq, PLC_LINK_ERR_BAD_VERSION, version);
        return;
    }

    switch ((PlcLinkCommand)cmd) {
        case PLC_LINK_CMD_PING:
            (void)send_ack(seq, cmd);
            break;

        case PLC_LINK_CMD_GET_STATUS:
            (void)send_status(seq);
            break;

        case PLC_LINK_CMD_GET_STATUS_EXT:
        case PLC_LINK_CMD_GET_NODE:
            if (!plc_link_ext_handle((PlcLinkCommand)cmd, seq, body, body_len)) {
                (void)send_error(seq, PLC_LINK_ERR_UNKNOWN_CMD, cmd);
            }
            break;

        case PLC_LINK_CMD_UPLOAD_BEGIN: {
            if (body_len != 12u) {
                (void)send_error(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
                break;
            }
            uint32_t graph_version = U32_LE(&body[0]);
            uint32_t total_size = U32_LE(&body[4]);
            uint32_t crc32 = U32_LE(&body[8]);
            PlcGraphLoaderResult r = plc_graph_loader_begin(graph_version, total_size, crc32);
            if (r == PLC_GRAPH_LOADER_OK) {
                (void)send_ack(seq, cmd);
            } else {
                (void)send_error(seq, PLC_LINK_ERR_GRAPH_LOADER, (uint32_t)r);
            }
            break;
        }

        case PLC_LINK_CMD_UPLOAD_CHUNK: {
            if (body_len < 4u) {
                (void)send_error(seq, PLC_LINK_ERR_BAD_SIZE, body_len);
                break;
            }
            uint32_t offset = U32_LE(&body[0]);
            const uint8_t* chunk = &body[4];
            uint16_t chunk_len = (uint16_t)(body_len - 4u);
            PlcGraphLoaderResult r = plc_graph_loader_chunk(offset, chunk, chunk_len);
            if (r == PLC_GRAPH_LOADER_OK) {
                (void)send_ack(seq, cmd);
            } else {
                (void)send_error(seq, PLC_LINK_ERR_GRAPH_LOADER, (uint32_t)r);
            }
            break;
        }

        case PLC_LINK_CMD_UPLOAD_END: {
            PlcGraphLoaderResult r = plc_graph_loader_end();
            if (r == PLC_GRAPH_LOADER_OK) {
                (void)send_ack(seq, cmd);
            } else {
                (void)send_error(seq, PLC_LINK_ERR_GRAPH_LOADER, (uint32_t)r);
            }
            break;
        }

        case PLC_LINK_CMD_ACTIVATE: {
            PlcGraphLoaderResult r = plc_graph_loader_activate();
            if (r != PLC_GRAPH_LOADER_OK) {
                (void)send_error(seq, PLC_LINK_ERR_GRAPH_LOADER, (uint32_t)r);
                break;
            }

            PlcGraphLoaderStatus status;
            plc_graph_loader_get_status(&status);

            const uint8_t* image = plc_graph_loader_get_active_image();
            uint32_t size = plc_graph_loader_get_active_image_size();

            if (image == NULL || size == 0u) {
                (void)send_error(seq, PLC_LINK_ERR_PERSIST, PLC_PERSIST_ERR_ARG);
                break;
            }

            if (!plc_persist_service_request_save(image, size, status.active_version)) {
                (void)send_error(seq, PLC_LINK_ERR_PERSIST_BUSY, 0u);
                break;
            }

            (void)send_ack(seq, cmd);
            break;
        }

        case PLC_LINK_CMD_SAFE_RESET:
            if (plc_ack_faults()) {
                (void)plc_request_run();
                (void)send_ack(seq, cmd);
            } else {
                (void)send_error(seq, PLC_LINK_ERR_SAFE_RESET_FAILED, (uint32_t)plc_get_state());
            }
            break;
        case PLC_LINK_CMD_FORCE_OUTPUT:
        case PLC_LINK_CMD_RELEASE_OUTPUT:
        case PLC_LINK_CMD_MEM_INFO:
        case PLC_LINK_CMD_MEM_READ:
        case PLC_LINK_CMD_MEM_WRITE:
            (void)plc_link_ext_handle((PlcLinkCommand)cmd, seq, body, body_len);
            break;

        default:
            (void)send_error(seq, PLC_LINK_ERR_UNKNOWN_CMD, cmd);
            break;
    }
}

bool plc_link_send_log(uint8_t level, const char* text)
{
    if (text == NULL) {
        return false;
    }

    uint8_t body[PLC_LINK_MAX_BODY_SIZE];
    size_t len = strnlen(text, PLC_LINK_MAX_BODY_SIZE - 1u);
    body[0] = level;
    memcpy(&body[1], text, len);
    return send_packet(PLC_LINK_RSP_LOG, 0u, body, (uint16_t)(len + 1u));
}

void plc_link_get_status(PlcLinkStatus* out_status)
{
    if (out_status != NULL) {
        *out_status = s_link.status;
    }
}
