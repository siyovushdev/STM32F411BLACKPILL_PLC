#include "friendly_plc/plc_log.h"

#include <stdarg.h>

static PlcLogCallback s_log_callback = 0;

void plc_log_set_callback(PlcLogCallback cb)
{
    s_log_callback = cb;
}

void plc_log_write(PlcLogLevel level, const char* tag, const char* fmt, ...)
{
    if (!s_log_callback) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    s_log_callback(level, tag, fmt, args);
    va_end(args);
}