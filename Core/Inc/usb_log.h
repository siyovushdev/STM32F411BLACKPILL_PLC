#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void UsbLog_Init(void);

void UsbLog_Put(const char* s, size_t n);
void UsbLog_Puts(const char* s);
int  UsbLog_Printf(const char* fmt, ...);

void UsbLog_PutFromISR(const char* s, size_t n);

void UsbLog_RegisterPlcLogger(void);

#ifdef __cplusplus
}
#endif