#include "plc_link_uart.h"
#include "friendly_plc/plc_safety.h"
#include "cmsis_os.h"
#include "message_buffer.h"
#include "semphr.h"
#include "stream_buffer.h"

#include <string.h>

#define PLC_UART_LEN_SIZE 2u
#define PLC_UART_CRC_SIZE 2u
#define PLC_UART_MAX_FRAME_SIZE (PLC_UART_LEN_SIZE + PLC_LINK_UART_MAX_PAYLOAD_SIZE + PLC_UART_CRC_SIZE)

typedef enum {
    RX_STATE_LEN_LO = 0,
    RX_STATE_LEN_HI,
    RX_STATE_PAYLOAD,
    RX_STATE_CRC_LO,
    RX_STATE_CRC_HI
} RxState;

typedef struct {
    UART_HandleTypeDef* huart;
    PlcLinkUartFrameCallback on_frame;
    void* user;

    StreamBufferHandle_t rx_stream;
    MessageBufferHandle_t tx_messages;
    SemaphoreHandle_t tx_done;
    SemaphoreHandle_t tx_mutex;

    uint8_t rx_dma_buf[PLC_LINK_UART_RX_DMA_BUFFER_SIZE];
    uint8_t tx_frame_buf[PLC_UART_MAX_FRAME_SIZE];
    uint8_t tx_build_buf[PLC_UART_MAX_FRAME_SIZE];

    RxState rx_state;
    uint16_t rx_expected_len;
    uint16_t rx_pos;
    uint16_t rx_received_crc;
    uint8_t rx_payload[PLC_LINK_UART_MAX_PAYLOAD_SIZE];

    volatile bool initialized;
    volatile bool rx_started;
    volatile bool tx_in_progress;

    PlcLinkUartStats stats;
} PlcLinkUartContext;

static PlcLinkUartContext s_ctx;

static uint16_t crc16_modbus(const uint8_t* data, size_t len)
{
    uint16_t crc = 0xFFFFu;

    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (uint8_t bit = 0; bit < 8u; bit++) {
            if ((crc & 0x0001u) != 0u) {
                crc = (uint16_t)((crc >> 1u) ^ 0xA001u);
            } else {
                crc = (uint16_t)(crc >> 1u);
            }
        }
    }

    return crc;
}

static void rx_parser_reset(void)
{
    s_ctx.rx_state = RX_STATE_LEN_LO;
    s_ctx.rx_expected_len = 0u;
    s_ctx.rx_pos = 0u;
    s_ctx.rx_received_crc = 0u;
}

static void rx_parser_feed(uint8_t b)
{
    volatile RxState debug_state = s_ctx.rx_state;
    volatile uint8_t debug_byte = b;
    (void)debug_state;
    (void)debug_byte;

    if (s_ctx.rx_state > RX_STATE_CRC_HI) {
        rx_parser_reset();
    }

    switch (s_ctx.rx_state) {
        case RX_STATE_LEN_LO:
            s_ctx.rx_expected_len = b;
            s_ctx.rx_state = RX_STATE_LEN_HI;
            break;

        case RX_STATE_LEN_HI:
            s_ctx.rx_expected_len |= (uint16_t)((uint16_t)b << 8u);
            if (s_ctx.rx_expected_len == 0u || s_ctx.rx_expected_len > PLC_LINK_UART_MAX_PAYLOAD_SIZE) {
                s_ctx.stats.rx_size_errors++;
                plc_fault_note_protocol_error(PLC_FAULT_PROTOCOL_FRAME, (int32_t)s_ctx.rx_expected_len);
                rx_parser_reset();
                return;
            }
            s_ctx.rx_pos = 0u;
            s_ctx.rx_state = RX_STATE_PAYLOAD;
            break;

        case RX_STATE_PAYLOAD:
            s_ctx.rx_payload[s_ctx.rx_pos++] = b;
            if (s_ctx.rx_pos >= s_ctx.rx_expected_len) {
                s_ctx.rx_state = RX_STATE_CRC_LO;
            }
            break;

        case RX_STATE_CRC_LO:
            s_ctx.rx_received_crc = b;
            s_ctx.rx_state = RX_STATE_CRC_HI;
            break;

        case RX_STATE_CRC_HI: {
            s_ctx.rx_received_crc |= (uint16_t)((uint16_t)b << 8u);

            uint8_t crc_header[2];
            crc_header[0] = (uint8_t)(s_ctx.rx_expected_len & 0xFFu);
            crc_header[1] = (uint8_t)((s_ctx.rx_expected_len >> 8u) & 0xFFu);

            uint16_t crc = crc16_modbus(crc_header, sizeof(crc_header));
            /* Continue Modbus CRC over payload without building a temporary frame. */
            for (uint16_t i = 0; i < s_ctx.rx_expected_len; i++) {
                uint8_t one = s_ctx.rx_payload[i];
                crc ^= (uint16_t)one;
                for (uint8_t bit = 0; bit < 8u; bit++) {
                    if ((crc & 0x0001u) != 0u) {
                        crc = (uint16_t)((crc >> 1u) ^ 0xA001u);
                    } else {
                        crc = (uint16_t)(crc >> 1u);
                    }
                }
            }

            if (crc == s_ctx.rx_received_crc) {
                s_ctx.stats.rx_frames_ok++;
                plc_fault_note_protocol_ok();
                if (s_ctx.on_frame != NULL) {
                    s_ctx.on_frame(s_ctx.rx_payload, s_ctx.rx_expected_len, s_ctx.user);
                }
            } else {
                s_ctx.stats.rx_crc_errors++;
                plc_fault_note_protocol_error(PLC_FAULT_PROTOCOL_CRC, (int32_t)s_ctx.rx_expected_len);
            }

            rx_parser_reset();
            break;
        }

        default:
            rx_parser_reset();
            break;
    }
}

bool plc_link_uart_init(const PlcLinkUartConfig* cfg)
{
    if (cfg == NULL || cfg->huart == NULL || cfg->on_frame == NULL) {
        return false;
    }

    memset(&s_ctx, 0, sizeof(s_ctx));

    s_ctx.huart = cfg->huart;
    s_ctx.on_frame = cfg->on_frame;
    s_ctx.user = cfg->user;

    s_ctx.rx_stream = xStreamBufferCreate(PLC_LINK_UART_RX_STREAM_SIZE, 1u);
    s_ctx.tx_messages = xMessageBufferCreate(PLC_LINK_UART_TX_MESSAGE_BUFFER_SIZE);
    s_ctx.tx_done = xSemaphoreCreateBinary();
    s_ctx.tx_mutex = xSemaphoreCreateMutex();

    if (s_ctx.rx_stream == NULL || s_ctx.tx_messages == NULL ||
        s_ctx.tx_done == NULL || s_ctx.tx_mutex == NULL) {
        return false;
    }

    rx_parser_reset();
    s_ctx.initialized = true;
    return true;
}

PlcLinkUartResult plc_link_uart_start_rx_dma(void)
{
    if (!s_ctx.initialized || s_ctx.huart == NULL) {
        return PLC_LINK_UART_ERR_NOT_INITIALIZED;
    }

    HAL_StatusTypeDef st = HAL_UARTEx_ReceiveToIdle_DMA(
        s_ctx.huart,
        s_ctx.rx_dma_buf,
        sizeof(s_ctx.rx_dma_buf)
    );

    if (st != HAL_OK) {
        s_ctx.stats.uart_errors++;
        plc_fault_note_protocol_error(PLC_FAULT_PROTOCOL_FRAME, PLC_LINK_UART_ERR_TIMEOUT);
        return PLC_LINK_UART_ERR_HAL;
    }

    if (s_ctx.huart->hdmarx != NULL) {
        __HAL_DMA_DISABLE_IT(s_ctx.huart->hdmarx, DMA_IT_HT);
    }

    s_ctx.rx_started = true;
    return PLC_LINK_UART_OK;
}

PlcLinkUartResult plc_link_uart_send(const uint8_t* payload, uint16_t payload_len, uint32_t timeout_ms)
{
    if (!s_ctx.initialized) {
        return PLC_LINK_UART_ERR_NOT_INITIALIZED;
    }
    if (payload == NULL) {
        return PLC_LINK_UART_ERR_NULL;
    }
    if (payload_len == 0u || payload_len > PLC_LINK_UART_MAX_PAYLOAD_SIZE) {
        return PLC_LINK_UART_ERR_SIZE;
    }

    TickType_t timeout = pdMS_TO_TICKS(timeout_ms);

    if (xSemaphoreTake(s_ctx.tx_mutex, timeout) != pdTRUE) {
        return PLC_LINK_UART_ERR_TIMEOUT;
    }

    uint16_t frame_len = (uint16_t)(PLC_UART_LEN_SIZE + payload_len + PLC_UART_CRC_SIZE);

    s_ctx.tx_build_buf[0] = (uint8_t)(payload_len & 0xFFu);
    s_ctx.tx_build_buf[1] = (uint8_t)((payload_len >> 8u) & 0xFFu);
    memcpy(&s_ctx.tx_build_buf[2], payload, payload_len);

    uint16_t crc = crc16_modbus(s_ctx.tx_build_buf, (size_t)(PLC_UART_LEN_SIZE + payload_len));
    s_ctx.tx_build_buf[2u + payload_len] = (uint8_t)(crc & 0xFFu);
    s_ctx.tx_build_buf[3u + payload_len] = (uint8_t)((crc >> 8u) & 0xFFu);

    size_t sent = xMessageBufferSend(s_ctx.tx_messages, s_ctx.tx_build_buf, frame_len, timeout);
    xSemaphoreGive(s_ctx.tx_mutex);

    if (sent != frame_len) {
        s_ctx.stats.tx_errors++;
        plc_fault_note_protocol_error(PLC_FAULT_PROTOCOL_FRAME, PLC_LINK_UART_ERR_TIMEOUT);
        return PLC_LINK_UART_ERR_TIMEOUT;
    }

    return PLC_LINK_UART_OK;
}

void plc_link_uart_rx_task(void* argument)
{
    (void)argument;

    uint8_t buf[128];

    if (s_ctx.initialized && !s_ctx.rx_started) {
        (void)plc_link_uart_start_rx_dma();
    }

    for (;;) {
        if (s_ctx.initialized && !s_ctx.rx_started) {
            (void)plc_link_uart_recover_rx();
        }

        size_t n = xStreamBufferReceive(
                s_ctx.rx_stream,
                buf,
                sizeof(buf),
                pdMS_TO_TICKS(100u)
        );

        for (size_t i = 0; i < n; i++) {
            rx_parser_feed(buf[i]);
        }
    }
}

void plc_link_uart_tx_task(void* argument)
{
    (void)argument;

    for (;;) {
        size_t n = xMessageBufferReceive(
            s_ctx.tx_messages,
            s_ctx.tx_frame_buf,
            sizeof(s_ctx.tx_frame_buf),
            portMAX_DELAY
        );

        if (n == 0u) {
            continue;
        }

        s_ctx.tx_in_progress = true;
        xSemaphoreTake(s_ctx.tx_done, 0u);

        HAL_StatusTypeDef st = HAL_UART_Transmit_DMA(s_ctx.huart, s_ctx.tx_frame_buf, (uint16_t)n);
        if (st != HAL_OK) {
            s_ctx.tx_in_progress = false;
            s_ctx.stats.tx_errors++;
            plc_fault_note_protocol_error(PLC_FAULT_PROTOCOL_FRAME, PLC_LINK_UART_ERR_TIMEOUT);
            continue;
        }

        if (xSemaphoreTake(s_ctx.tx_done, pdMS_TO_TICKS(1000u)) != pdTRUE) {
            s_ctx.tx_in_progress = false;
            s_ctx.stats.tx_errors++;
            plc_fault_note_protocol_error(PLC_FAULT_PROTOCOL_FRAME, PLC_LINK_UART_ERR_TIMEOUT);
            (void)HAL_UART_AbortTransmit(s_ctx.huart);
            continue;
        }

        s_ctx.tx_in_progress = false;
        s_ctx.stats.tx_frames++;
    }
}

void plc_link_uart_get_stats(PlcLinkUartStats* out_stats)
{
    if (out_stats != NULL) {
        *out_stats = s_ctx.stats;
    }
}

void plc_link_uart_hal_rx_event_callback(UART_HandleTypeDef* huart, uint16_t size)
{
    if (!s_ctx.initialized || huart != s_ctx.huart || size == 0u) {
        return;
    }

    if (size > sizeof(s_ctx.rx_dma_buf)) {
        size = sizeof(s_ctx.rx_dma_buf);
    }

    BaseType_t hpw = pdFALSE;
    size_t sent = xStreamBufferSendFromISR(s_ctx.rx_stream, s_ctx.rx_dma_buf, size, &hpw);
    if (sent != size) {
        s_ctx.stats.rx_overflows++;
        plc_fault_note_protocol_error(PLC_FAULT_PROTOCOL_FRAME, (int32_t)size);
    } else {
        s_ctx.stats.rx_bytes += size;
    }

    (void)HAL_UARTEx_ReceiveToIdle_DMA(s_ctx.huart, s_ctx.rx_dma_buf, sizeof(s_ctx.rx_dma_buf));
    if (s_ctx.huart->hdmarx != NULL) {
        __HAL_DMA_DISABLE_IT(s_ctx.huart->hdmarx, DMA_IT_HT);
    }

    portYIELD_FROM_ISR(hpw);
}

void plc_link_uart_hal_tx_cplt_callback(UART_HandleTypeDef* huart)
{
    if (!s_ctx.initialized || huart != s_ctx.huart) {
        return;
    }

    BaseType_t hpw = pdFALSE;
    xSemaphoreGiveFromISR(s_ctx.tx_done, &hpw);
    portYIELD_FROM_ISR(hpw);
}

void plc_link_uart_hal_error_callback(UART_HandleTypeDef* huart)
{
    if (huart != s_ctx.huart) {
        return;
    }

    s_ctx.stats.uart_errors++;
    s_ctx.rx_started = false;

    (void)plc_link_uart_recover_rx();
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size)
{
    plc_link_uart_hal_rx_event_callback(huart, Size);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    plc_link_uart_hal_tx_cplt_callback(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    plc_link_uart_hal_error_callback(huart);
}

PlcLinkUartResult plc_link_uart_recover_rx(void)
{
    if (!s_ctx.initialized || s_ctx.huart == NULL) {
        return PLC_LINK_UART_ERR_NOT_INITIALIZED;
    }

//    taskENTER_CRITICAL();

    s_ctx.rx_started = false;

    rx_parser_reset();

    if (s_ctx.rx_stream != NULL) {
        xStreamBufferReset(s_ctx.rx_stream);
    }

    if (s_ctx.huart->hdmarx != NULL) {
        __HAL_DMA_DISABLE_IT(s_ctx.huart->hdmarx, DMA_IT_HT);
    }

    taskEXIT_CRITICAL();

    (void)HAL_UART_AbortReceive(s_ctx.huart);
    (void)HAL_UART_DMAStop(s_ctx.huart);

    HAL_StatusTypeDef st = HAL_UARTEx_ReceiveToIdle_DMA(
            s_ctx.huart,
            s_ctx.rx_dma_buf,
            sizeof(s_ctx.rx_dma_buf)
    );

    if (st != HAL_OK) {
        s_ctx.stats.uart_errors++;
        return PLC_LINK_UART_ERR_HAL;
    }

    if (s_ctx.huart->hdmarx != NULL) {
        __HAL_DMA_DISABLE_IT(s_ctx.huart->hdmarx, DMA_IT_HT);
    }

    s_ctx.rx_started = true;
    return PLC_LINK_UART_OK;
}