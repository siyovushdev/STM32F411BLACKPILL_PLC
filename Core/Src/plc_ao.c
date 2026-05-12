#include "plc_ao.h"
#include <math.h>
#include <string.h>

static float clampf(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static uint16_t clamp_u16(int32_t x)
{
    if (x < 0) return 0;
    if (x > 65535) return 65535;
    return (uint16_t)x;
}

void PlcAo_Init(PlcAo* ao)
{
    if (ao == NULL) return;

    memset(ao, 0, sizeof(*ao));

    ao->cal.range = PLC_AO_RANGE_0_10V;
    ao->cal.invert = false;
    ao->cal.offset_code = 32768.0f;
    ao->cal.gain_code_per_volt = 65535.0f / 20.0f;
    ao->last_status = HAL_OK;

    for (uint8_t i = 0; i < 8u; i++) {
        ao->last_code[i] = 0x8000u;
        ao->last_percent[i] = 0.0f;
    }
}

void PlcAo_SetCal(PlcAo* ao, const PlcAoCal* cal)
{
    if (ao == NULL || cal == NULL) return;
    ao->cal = *cal;
}

HAL_StatusTypeDef PlcAo_SetRawU16(uint8_t ch, uint16_t code)
{
    return DAC8568_SetCh_Code(ch, code);
}

HAL_StatusTypeDef PlcAo_SetVoltage(PlcAo* ao, uint8_t ch, float volts)
{
    if (ao == NULL || ch > 7u) return HAL_ERROR;

    float v = clampf(volts, 0.0f, 10.0f);
    if (ao->cal.invert) {
        v = 10.0f - v;
    }

    const float code_f = ao->cal.offset_code + ao->cal.gain_code_per_volt * v;
    const uint16_t code = clamp_u16((int32_t)lroundf(code_f));

    HAL_StatusTypeDef st = PlcAo_SetRawU16(ch, code);
    ao->last_status = st;
    if (st == HAL_OK) {
        ao->last_code[ch] = code;
        ao->last_percent[ch] = (volts / 10.0f) * 100.0f;
    }

    return st;
}

HAL_StatusTypeDef PlcAo_SetPercent(PlcAo* ao, uint8_t ch, float percent)
{
    const float p = clampf(percent, 0.0f, 100.0f);
    const float v = (p / 100.0f) * 10.0f;
    return PlcAo_SetVoltage(ao, ch, v);
}

HAL_StatusTypeDef PlcAo_SetRawSiemens(PlcAo* ao, uint8_t ch, int32_t raw27648)
{
    if (raw27648 < 0) raw27648 = 0;
    if (raw27648 > 27648) raw27648 = 27648;

    const float v = ((float)raw27648 * 10.0f) / 27648.0f;
    return PlcAo_SetVoltage(ao, ch, v);
}

HAL_StatusTypeDef PlcAo_ResetAllToZeroVolt(PlcAo* ao)
{
    HAL_StatusTypeDef st = DAC8568_ResetOutputsToZeroVolt();

    if (ao != NULL) {
        ao->last_status = st;
        if (st == HAL_OK) {
            for (uint8_t i = 0; i < 8u; i++) {
                ao->last_code[i] = 0x8000u;
                ao->last_percent[i] = 0.0f;
            }
        }
    }

    return st;
}
