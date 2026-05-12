#include "plc_ai.h"
#include <string.h>

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static bool valid_ch(uint8_t ch)
{
    return ch < PLC_AI_CH_COUNT;
}

static void init_channel_defaults(PlcAi *ai, uint8_t ch)
{
    ai->calib[ch].raw_lo = 0;
    ai->calib[ch].raw_hi = 32767;
    ai->calib[ch].eng_lo = 0.0f;
    ai->calib[ch].eng_hi = 10.0f;
    ai->calib[ch].enabled = true;

    ai->filter[ch].enabled = true;
    ai->filter[ch].alpha = 0.2f;

    ai->snapshot.ch[ch].raw = 0;
    ai->snapshot.ch[ch].value = 0.0f;
    ai->snapshot.ch[ch].raw_to_value = 0.0f;
    ai->snapshot.ch[ch].valid = false;
}

void PlcAi_Init(PlcAi *ai, AD7606_Handle *adc)
{
    if (ai == NULL) return;

    memset(ai, 0, sizeof(*ai));
    ai->adc = adc;
    ai->channel_count = PLC_AI_CH_COUNT;
    ai->last_status = HAL_OK;

    for (uint8_t i = 0; i < PLC_AI_CH_COUNT; i++) {
        init_channel_defaults(ai, i);
    }
}

float PlcAi_RawToVoltsIdeal_PM10(int16_t raw)
{
    return ((float)raw) * (10.0f / 32768.0f);
}

float PlcAi_RawToEngineering(const PlcAi *ai, uint8_t ch, int16_t raw)
{
    if (ai == NULL || !valid_ch(ch)) return 0.0f;

    const PlcAiCalibration *c = &ai->calib[ch];

    float y;
    if (!c->enabled) {
        y = PlcAi_RawToVoltsIdeal_PM10(raw);
    } else {
        const int32_t dx = (int32_t)c->raw_hi - (int32_t)c->raw_lo;
        if (dx == 0) return c->eng_lo;
        const float k = (c->eng_hi - c->eng_lo) / (float)dx;
        y = c->eng_lo + ((float)((int32_t)raw - (int32_t)c->raw_lo) * k);
    }

    return clampf(y, 0.0f, 10.0f);
}

static float apply_filter(const PlcAiFilterConfig *f, bool prev_valid, float prev_value, float new_value)
{
    if (f == NULL || !f->enabled) return new_value;
    const float alpha = clampf(f->alpha, 0.0f, 1.0f);
    if (!prev_valid) return new_value;
    return prev_value + alpha * (new_value - prev_value);
}

HAL_StatusTypeDef PlcAi_Read(PlcAi *ai)
{
    if (ai == NULL || ai->adc == NULL) return HAL_ERROR;

    int16_t raw[PLC_AI_CH_COUNT] = {0};
    HAL_StatusTypeDef st = AD7606_ReadSample(ai->adc, raw);
    ai->last_status = st;

    if (st != HAL_OK) {
        ai->read_err_count++;
        if (st == HAL_TIMEOUT) ai->timeout_count++;
        ai->snapshot.valid = false;
        for (uint8_t ch = 0; ch < ai->channel_count; ch++) {
            ai->snapshot.ch[ch].valid = false;
        }
        return st;
    }

    ai->read_ok_count++;
    ai->snapshot.valid = true;
    ai->snapshot.timestamp_ms = HAL_GetTick();
    ai->snapshot.sample_counter++;

    for (uint8_t ch = 0; ch < ai->channel_count; ch++) {
        PlcAiChannelSnapshot *dst = &ai->snapshot.ch[ch];
        const float eng = PlcAi_RawToEngineering(ai, ch, raw[ch]);
        const float filtered = apply_filter(&ai->filter[ch], dst->valid, dst->value, eng);

        dst->raw = raw[ch];
        dst->raw_to_value = eng;
        dst->value = filtered;
        dst->valid = true;
    }

    return HAL_OK;
}

const PlcAiSnapshot* PlcAi_GetSnapshot(const PlcAi *ai)
{
    return ai == NULL ? NULL : &ai->snapshot;
}

int16_t PlcAi_GetRaw(const PlcAi *ai, uint8_t ch)
{
    if (ai == NULL || !valid_ch(ch)) return 0;
    return ai->snapshot.ch[ch].raw;
}

float PlcAi_GetValue(const PlcAi *ai, uint8_t ch)
{
    if (ai == NULL || !valid_ch(ch)) return 0.0f;
    return ai->snapshot.ch[ch].value;
}

bool PlcAi_IsValid(const PlcAi *ai, uint8_t ch)
{
    if (ai == NULL || !valid_ch(ch)) return false;
    return ai->snapshot.valid && ai->snapshot.ch[ch].valid;
}

void PlcAi_SetCalibration(PlcAi *ai, uint8_t ch, int16_t raw_lo, float eng_lo, int16_t raw_hi, float eng_hi)
{
    if (ai == NULL || !valid_ch(ch)) return;
    ai->calib[ch].raw_lo = raw_lo;
    ai->calib[ch].raw_hi = raw_hi;
    ai->calib[ch].eng_lo = eng_lo;
    ai->calib[ch].eng_hi = eng_hi;
    ai->calib[ch].enabled = raw_hi != raw_lo;
}

void PlcAi_SetCalibrationDefault_0_10V(PlcAi *ai, uint8_t ch, int16_t raw0, int16_t raw10)
{
    PlcAi_SetCalibration(ai, ch, raw0, 0.0f, raw10, 10.0f);
}

void PlcAi_SetFilterEma(PlcAi *ai, uint8_t ch, bool enabled, float alpha)
{
    if (ai == NULL || !valid_ch(ch)) return;
    ai->filter[ch].enabled = enabled;
    ai->filter[ch].alpha = clampf(alpha, 0.0f, 1.0f);
}
