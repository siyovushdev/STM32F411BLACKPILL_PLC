#pragma once

#include "ad7606.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLC_AI_CH_COUNT AD7606_CH_COUNT

typedef struct {
    int16_t raw;
    float value;
    float raw_to_value;
    bool valid;
} PlcAiChannelSnapshot;

typedef struct {
    uint32_t timestamp_ms;
    uint32_t sample_counter;
    bool valid;
    PlcAiChannelSnapshot ch[PLC_AI_CH_COUNT];
} PlcAiSnapshot;

typedef struct {
    int16_t raw_lo;
    int16_t raw_hi;
    float eng_lo;
    float eng_hi;
    bool enabled;
} PlcAiCalibration;

typedef struct {
    bool enabled;
    float alpha;
} PlcAiFilterConfig;

typedef struct {
    AD7606_Handle *adc;
    uint8_t channel_count;
    PlcAiCalibration calib[PLC_AI_CH_COUNT];
    PlcAiFilterConfig filter[PLC_AI_CH_COUNT];
    PlcAiSnapshot snapshot;
    uint32_t read_ok_count;
    uint32_t read_err_count;
    uint32_t timeout_count;
    HAL_StatusTypeDef last_status;
} PlcAi;

void PlcAi_Init(PlcAi *ai, AD7606_Handle *adc);
HAL_StatusTypeDef PlcAi_Read(PlcAi *ai);
const PlcAiSnapshot* PlcAi_GetSnapshot(const PlcAi *ai);
int16_t PlcAi_GetRaw(const PlcAi *ai, uint8_t ch);
float PlcAi_GetValue(const PlcAi *ai, uint8_t ch);
bool PlcAi_IsValid(const PlcAi *ai, uint8_t ch);
void PlcAi_SetCalibration(PlcAi *ai, uint8_t ch, int16_t raw_lo, float eng_lo, int16_t raw_hi, float eng_hi);
void PlcAi_SetCalibrationDefault_0_10V(PlcAi *ai, uint8_t ch, int16_t raw0, int16_t raw10);
void PlcAi_SetFilterEma(PlcAi *ai, uint8_t ch, bool enabled, float alpha);
float PlcAi_RawToEngineering(const PlcAi *ai, uint8_t ch, int16_t raw);
float PlcAi_RawToVoltsIdeal_PM10(int16_t raw);

#ifdef __cplusplus
}
#endif
