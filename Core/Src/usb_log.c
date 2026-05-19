#include "usb_log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "usbd_cdc_if.h"
#include "usbd_def.h"

#include "friendly_plc/plc_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#ifndef USB_LOG_BUF_SIZE
#define USB_LOG_BUF_SIZE 1024u
#endif

#ifndef USB_LOG_TX_CHUNK_SIZE
#define USB_LOG_TX_CHUNK_SIZE 128u
#endif

#ifndef USB_LOG_TASK_STACK_WORDS
#define USB_LOG_TASK_STACK_WORDS 384u
#endif

#ifndef USB_LOG_TASK_PRIORITY
#define USB_LOG_TASK_PRIORITY (tskIDLE_PRIORITY + 1u)
#endif

#ifndef USB_LOG_TX_BUSY_TIMEOUT_MS
#define USB_LOG_TX_BUSY_TIMEOUT_MS 20u
#endif

#define USB_LOG_NOTIFY_VALUE 0x01u

static uint8_t s_ring[USB_LOG_BUF_SIZE];

static volatile size_t s_write_pos = 0u;
static volatile size_t s_read_pos = 0u;

static SemaphoreHandle_t s_mutex = NULL;
static TaskHandle_t s_task = NULL;

extern USBD_HandleTypeDef hUsbDeviceFS;

static bool UsbLog_CdcReady(void)
{
    return hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED;
}

static size_t UsbLog_SpaceUnlocked(void)
{
    const size_t w = s_write_pos;
    const size_t r = s_read_pos;

    if (r <= w) {
        return USB_LOG_BUF_SIZE - (w - r) - 1u;
    }

    return r - w - 1u;
}

static void UsbLog_PushUnlocked(const uint8_t* data, size_t len)
{
    while (len > 0u) {
        s_ring[s_write_pos] = *data++;
        s_write_pos = (s_write_pos + 1u) % USB_LOG_BUF_SIZE;
        len--;
    }
}

static size_t UsbLog_PullUnlocked(uint8_t* out, size_t max_len)
{
    size_t count = 0u;

    while ((s_read_pos != s_write_pos) && (count < max_len)) {
        out[count++] = s_ring[s_read_pos];
        s_read_pos = (s_read_pos + 1u) % USB_LOG_BUF_SIZE;
    }

    return count;
}

static void UsbLog_NotifyTask(void)
{
    if (s_task != NULL) {
        xTaskNotify(s_task, USB_LOG_NOTIFY_VALUE, eSetBits);
    }
}

static void UsbLog_NotifyTaskFromISR(void)
{
    if (s_task != NULL) {
        BaseType_t hpw = pdFALSE;
        xTaskNotifyFromISR(s_task, USB_LOG_NOTIFY_VALUE, eSetBits, &hpw);
        portYIELD_FROM_ISR(hpw);
    }
}

static void UsbLog_Task(void* argument)
{
    (void)argument;

    uint8_t chunk[USB_LOG_TX_CHUNK_SIZE];

    for (;;) {
        (void)xTaskNotifyWait(0u, USB_LOG_NOTIFY_VALUE, NULL, pdMS_TO_TICKS(10u));

        for (;;) {
            size_t len = 0u;

            if (s_mutex != NULL && xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
                len = UsbLog_PullUnlocked(chunk, sizeof(chunk));
                xSemaphoreGive(s_mutex);
            }

            if (len == 0u) {
                break;
            }

            if (!UsbLog_CdcReady()) {
                if (s_mutex != NULL && xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
                    for (size_t i = len; i > 0u; i--) {
                        s_read_pos = (s_read_pos == 0u) ? (USB_LOG_BUF_SIZE - 1u) : (s_read_pos - 1u);
                        s_ring[s_read_pos] = chunk[i - 1u];
                    }
                    xSemaphoreGive(s_mutex);
                }

                vTaskDelay(pdMS_TO_TICKS(50u));
                break;
            }

            const TickType_t start = xTaskGetTickCount();

            while (CDC_Transmit_FS(chunk, (uint16_t)len) == USBD_BUSY) {
                vTaskDelay(pdMS_TO_TICKS(1u));

                if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(USB_LOG_TX_BUSY_TIMEOUT_MS)) {
                    break;
                }
            }
        }
    }
}

void UsbLog_Init(void)
{
    if (s_task != NULL) {
        return;
    }

    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        return;
    }

    (void)xTaskCreate(
            UsbLog_Task,
            "usbLog",
            USB_LOG_TASK_STACK_WORDS,
            NULL,
            USB_LOG_TASK_PRIORITY,
            &s_task
    );
}

void UsbLog_Put(const char* s, size_t n)
{
    if (s == NULL || n == 0u || s_mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return;
    }

    size_t free_space = UsbLog_SpaceUnlocked();

    if (n > free_space) {
        n = free_space;
    }

    if (n > 0u) {
        UsbLog_PushUnlocked((const uint8_t*)s, n);
    }

    xSemaphoreGive(s_mutex);

    UsbLog_NotifyTask();
}

void UsbLog_Puts(const char* s)
{
    if (s == NULL) {
        return;
    }

    UsbLog_Put(s, strlen(s));
}

int UsbLog_Printf(const char* fmt, ...)
{
    if (fmt == NULL) {
        return -1;
    }

    char buf[256];

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (n < 0) {
        return n;
    }

    size_t len = (size_t)n;

    if (len >= sizeof(buf)) {
        len = sizeof(buf) - 1u;
    }

    UsbLog_Put(buf, len);
    return n;
}

void UsbLog_PutFromISR(const char* s, size_t n)
{
    if (s == NULL || n == 0u) {
        return;
    }

    UBaseType_t saved_mask = taskENTER_CRITICAL_FROM_ISR();

    size_t free_space = UsbLog_SpaceUnlocked();

    if (n > free_space) {
        n = free_space;
    }

    if (n > 0u) {
        UsbLog_PushUnlocked((const uint8_t*)s, n);
    }

    taskEXIT_CRITICAL_FROM_ISR(saved_mask);

    UsbLog_NotifyTaskFromISR();
}

static const char* UsbLog_LevelName(PlcLogLevel level)
{
    switch (level) {
        case PLC_LOG_LEVEL_TRACE: return "T";
        case PLC_LOG_LEVEL_DEBUG: return "D";
        case PLC_LOG_LEVEL_INFO:  return "I";
        case PLC_LOG_LEVEL_WARN:  return "W";
        case PLC_LOG_LEVEL_ERROR: return "E";
        default:                  return "?";
    }
}

static void UsbLog_PlcCallback(PlcLogLevel level,
                               const char* tag,
                               const char* fmt,
                               va_list args)
{
    char line[256];

    const uint32_t ms = (uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS;

    int prefix_len = snprintf(
            line,
            sizeof(line),
            "[%lu][%s][%s] ",
            (unsigned long)ms,
            UsbLog_LevelName(level),
            tag != NULL ? tag : "PLC"
    );

    if (prefix_len < 0) {
        return;
    }

    if (prefix_len >= (int)sizeof(line)) {
        prefix_len = (int)sizeof(line) - 1;
    }

    int body_len = vsnprintf(
            line + prefix_len,
            sizeof(line) - (size_t)prefix_len,
            fmt != NULL ? fmt : "",
            args
    );

    if (body_len < 0) {
        return;
    }

    size_t total_len = (size_t)prefix_len + (size_t)body_len;

    if (total_len >= sizeof(line) - 3u) {
        total_len = sizeof(line) - 3u;
    }

    line[total_len++] = '\r';
    line[total_len++] = '\n';
    line[total_len] = '\0';

    UsbLog_Put(line, total_len);
}

void UsbLog_RegisterPlcLogger(void)
{
    plc_log_set_callback(UsbLog_PlcCallback);
}