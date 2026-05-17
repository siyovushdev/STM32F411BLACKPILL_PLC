#pragma once

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PLC_LINK_UART_MAX_PAYLOAD_SIZE
#define PLC_LINK_UART_MAX_PAYLOAD_SIZE 1024u
#endif

#ifndef PLC_LINK_UART_RX_DMA_BUFFER_SIZE
#define PLC_LINK_UART_RX_DMA_BUFFER_SIZE 2048u
#endif

#ifndef PLC_LINK_UART_RX_STREAM_SIZE
#define PLC_LINK_UART_RX_STREAM_SIZE 8192u
#endif

#ifndef PLC_LINK_UART_TX_MESSAGE_BUFFER_SIZE
#define PLC_LINK_UART_TX_MESSAGE_BUFFER_SIZE 4096u
#endif

typedef enum {
    PLC_LINK_UART_OK = 0,
    PLC_LINK_UART_ERR_NULL,
    PLC_LINK_UART_ERR_NOT_INITIALIZED,
    PLC_LINK_UART_ERR_SIZE,
    PLC_LINK_UART_ERR_BUSY,
    PLC_LINK_UART_ERR_TIMEOUT,
    PLC_LINK_UART_ERR_HAL,
    PLC_LINK_UART_ERR_OS
} PlcLinkUartResult;

typedef void (*PlcLinkUartFrameCallback)(const uint8_t* payload, uint16_t payload_len, void* user);

typedef struct {
    UART_HandleTypeDef* huart;
    PlcLinkUartFrameCallback on_frame;
    void* user;
} PlcLinkUartConfig;

typedef struct {
    uint32_t rx_bytes;
    uint32_t rx_frames_ok;
    uint32_t rx_crc_errors;
    uint32_t rx_size_errors;
    uint32_t rx_overflows;
    uint32_t tx_frames;
    uint32_t tx_errors;
    uint32_t uart_errors;
} PlcLinkUartStats;

bool plc_link_uart_init(const PlcLinkUartConfig* cfg);
PlcLinkUartResult plc_link_uart_start_rx_dma(void);

void plc_link_uart_rx_task(void* argument);
void plc_link_uart_tx_task(void* argument);

PlcLinkUartResult plc_link_uart_send(const uint8_t* payload, uint16_t payload_len, uint32_t timeout_ms);
void plc_link_uart_get_stats(PlcLinkUartStats* out_stats);

void plc_link_uart_hal_rx_event_callback(UART_HandleTypeDef* huart, uint16_t size);
void plc_link_uart_hal_tx_cplt_callback(UART_HandleTypeDef* huart);
void plc_link_uart_hal_error_callback(UART_HandleTypeDef* huart);

PlcLinkUartResult plc_link_uart_recover_rx(void);

#ifdef __cplusplus
}
#endif
