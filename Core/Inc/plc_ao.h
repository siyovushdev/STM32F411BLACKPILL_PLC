#pragma once

#include "dac8568.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLC_AO_RANGE_0_10V = 0,
} PlcAoRange;

typedef struct {
    float offset_code;
    float gain_code_per_volt;
    bool invert;
    PlcAoRange range;
} PlcAoCal;

typedef struct {
    PlcAoCal cal;
    uint16_t last_code[8];
    float last_percent[8];
    HAL_StatusTypeDef last_status;
} PlcAo;

void PlcAo_Init(PlcAo* ao);
void PlcAo_SetCal(PlcAo* ao, const PlcAoCal* cal);
HAL_StatusTypeDef PlcAo_SetVoltage(PlcAo* ao, uint8_t ch, float volts);
HAL_StatusTypeDef PlcAo_SetPercent(PlcAo* ao, uint8_t ch, float percent);
HAL_StatusTypeDef PlcAo_SetRawU16(uint8_t ch, uint16_t code);
HAL_StatusTypeDef PlcAo_SetRawSiemens(PlcAo* ao, uint8_t ch, int32_t raw27648);
HAL_StatusTypeDef PlcAo_ResetAllToZeroVolt(PlcAo* ao);

#ifdef __cplusplus
}
#endif
