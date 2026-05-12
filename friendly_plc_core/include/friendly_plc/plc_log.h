#pragma once

#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLC_LOG_LEVEL_TRACE = 0,
    PLC_LOG_LEVEL_DEBUG = 1,
    PLC_LOG_LEVEL_INFO  = 2,
    PLC_LOG_LEVEL_WARN  = 3,
    PLC_LOG_LEVEL_ERROR = 4
} PlcLogLevel;

typedef void (*PlcLogCallback)(
        PlcLogLevel level,
        const char* tag,
        const char* fmt,
        va_list args
);

void plc_log_set_callback(PlcLogCallback cb);
void plc_log_write(PlcLogLevel level, const char* tag, const char* fmt, ...);

#define PLC_LOGT(tag, fmt, ...) plc_log_write(PLC_LOG_LEVEL_TRACE, tag, fmt, ##__VA_ARGS__)
#define PLC_LOGD(tag, fmt, ...) plc_log_write(PLC_LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#define PLC_LOGI(tag, fmt, ...) plc_log_write(PLC_LOG_LEVEL_INFO,  tag, fmt, ##__VA_ARGS__)
#define PLC_LOGW(tag, fmt, ...) plc_log_write(PLC_LOG_LEVEL_WARN,  tag, fmt, ##__VA_ARGS__)
#define PLC_LOGE(tag, fmt, ...) plc_log_write(PLC_LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif