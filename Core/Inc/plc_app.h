#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool plc_app_init(void);
bool plc_app_create_tasks(void);

void StartPlcScanTask(void* argument);
void StartLinkRxTask(void* argument);
void StartLinkTxTask(void* argument);
void StartDiagTask(void* argument);

#ifdef __cplusplus
}
#endif
