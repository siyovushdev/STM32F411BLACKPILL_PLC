#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLC_LINK_PROTO_VERSION 1u
#define PLC_LINK_MAX_BODY_SIZE 1018u

typedef enum {
    PLC_LINK_CMD_PING = 0x01,
    PLC_LINK_CMD_GET_STATUS = 0x02,
    PLC_LINK_CMD_GET_STATUS_EXT = 0x03,
    PLC_LINK_CMD_GET_NODE = 0x04,

    PLC_LINK_CMD_FORCE_OUTPUT = 0x05,
    PLC_LINK_CMD_RELEASE_OUTPUT = 0x06,

    PLC_LINK_CMD_MEM_INFO = 0x07,
    PLC_LINK_CMD_MEM_READ = 0x08,
    PLC_LINK_CMD_MEM_WRITE = 0x09,

    PLC_LINK_CMD_UPLOAD_BEGIN = 0x10,
    PLC_LINK_CMD_UPLOAD_CHUNK = 0x11,
    PLC_LINK_CMD_UPLOAD_END = 0x12,
    PLC_LINK_CMD_ACTIVATE = 0x13,
    PLC_LINK_CMD_UPLOAD_CANCEL = 0x14,

    PLC_LINK_CMD_SAFE_RESET = 0x20,

    PLC_LINK_CMD_GET_STATUS_WEB_V2 = 0x40,
    PLC_LINK_CMD_GET_NODES_SNAPSHOT = 0x41,
    PLC_LINK_CMD_GET_IO_SUMMARY = 0x42,


    PLC_LINK_RSP_ACK = 0x80,
    PLC_LINK_RSP_ERROR = 0x81,
    PLC_LINK_RSP_STATUS = 0x82,
    PLC_LINK_RSP_LOG = 0x83,
    PLC_LINK_RSP_STATUS_EXT = 0x84,
    PLC_LINK_RSP_NODE = 0x85,
    PLC_LINK_RSP_MEM_INFO = 0x86,
    PLC_LINK_RSP_MEM_READ = 0x87,

    PLC_LINK_RSP_STATUS_WEB_V2 = 0xC0,
    PLC_LINK_RSP_NODES_SNAPSHOT = 0xC1,
    PLC_LINK_RSP_IO_SUMMARY = 0xC2,
} PlcLinkCommand;

typedef enum {
    PLC_LINK_ERR_OK = 0,
    PLC_LINK_ERR_BAD_SIZE = 1,
    PLC_LINK_ERR_BAD_VERSION = 2,
    PLC_LINK_ERR_UNKNOWN_CMD = 3,
    PLC_LINK_ERR_GRAPH_LOADER = 4,
    PLC_LINK_ERR_TX = 5,
    PLC_LINK_ERR_PERSIST = 6,
    PLC_LINK_ERR_PERSIST_BUSY = 7,
    PLC_LINK_ERR_BAD_INDEX = 8,
    PLC_LINK_ERR_NOT_FOUND = 9,
    PLC_LINK_ERR_SAFE_RESET_FAILED = 10,
    PLC_LINK_ERR_BAD_MEM_TYPE = 11,
    PLC_LINK_ERR_BAD_MEM_RANGE = 12,
    PLC_LINK_ERR_BAD_MEM_WRITE = 13
} PlcLinkError;

typedef struct {
    uint32_t rx_frames;
    uint32_t tx_frames;
    uint32_t errors;
    uint16_t last_seq;
    uint8_t last_error;
} PlcLinkStatus;

void plc_link_init(void);
void plc_link_on_frame(const uint8_t* payload, uint16_t payload_len, void* user);
bool plc_link_send_log(uint8_t level, const char* text);
void plc_link_get_status(PlcLinkStatus* out_status);

/* Used by extension handlers. Keeps all UART framing in plc_link.c. */
bool plc_link_send_response(uint8_t rsp_cmd, uint16_t seq, const uint8_t* body, uint16_t body_len);
bool plc_link_send_error_response(uint16_t seq, uint8_t error_code, uint32_t detail);

#ifdef __cplusplus
}
#endif
