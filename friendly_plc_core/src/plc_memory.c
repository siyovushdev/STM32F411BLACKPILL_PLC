#include "friendly_plc/plc_memory.h"

#include <string.h>

static bool    s_memBool[PLC_MEM_BOOL_COUNT];
static int32_t s_memInt[PLC_MEM_INT_COUNT];
static float   s_memReal[PLC_MEM_REAL_COUNT];

static PlcMemoryResult mem_count_for_type(PlcMemoryType type, uint16_t* out_count)
{
    if (out_count == NULL) {
        return PLC_MEM_ERR_BAD_ARG;
    }

    switch (type) {
        case PLC_MEM_TYPE_BOOL:
            *out_count = PLC_MEM_BOOL_COUNT;
            return PLC_MEM_OK;

        case PLC_MEM_TYPE_INT:
            *out_count = PLC_MEM_INT_COUNT;
            return PLC_MEM_OK;

        case PLC_MEM_TYPE_REAL:
            *out_count = PLC_MEM_REAL_COUNT;
            return PLC_MEM_OK;

        default:
            *out_count = 0u;
            return PLC_MEM_ERR_BAD_TYPE;
    }
}

void plc_mem_get_info(PlcMemoryInfo* out)
{
    if (out == NULL) {
        return;
    }

    out->bool_count = PLC_MEM_BOOL_COUNT;
    out->int_count = PLC_MEM_INT_COUNT;
    out->real_count = PLC_MEM_REAL_COUNT;

    out->bool_size = sizeof(bool);
    out->int_size = sizeof(int32_t);
    out->real_size = sizeof(float);
}

PlcMemoryResult plc_mem_get_count(PlcMemoryType type, uint16_t* out_count)
{
    return mem_count_for_type(type, out_count);
}

PlcMemoryResult plc_mem_validate_range(PlcMemoryType type, uint16_t index, uint16_t count)
{
    uint16_t total = 0u;
    PlcMemoryResult r = mem_count_for_type(type, &total);
    if (r != PLC_MEM_OK) {
        return r;
    }

    if (count == 0u) {
        return PLC_MEM_ERR_BAD_RANGE;
    }

    if (index >= total) {
        return PLC_MEM_ERR_BAD_INDEX;
    }

    if (count > (uint16_t)(total - index)) {
        return PLC_MEM_ERR_BAD_RANGE;
    }

    return PLC_MEM_OK;
}

void plc_mem_set_bool(uint16_t index, bool value)
{
    if (index >= PLC_MEM_BOOL_COUNT) {
        return;
    }

    s_memBool[index] = value;
}

bool plc_mem_get_bool(uint16_t index)
{
    if (index >= PLC_MEM_BOOL_COUNT) {
        return false;
    }

    return s_memBool[index];
}

void plc_mem_set_int(uint16_t index, int32_t value)
{
    if (index >= PLC_MEM_INT_COUNT) {
        return;
    }

    s_memInt[index] = value;
}

int32_t plc_mem_get_int(uint16_t index)
{
    if (index >= PLC_MEM_INT_COUNT) {
        return 0;
    }

    return s_memInt[index];
}

void plc_mem_set_real(uint16_t index, float value)
{
    if (index >= PLC_MEM_REAL_COUNT) {
        return;
    }

    s_memReal[index] = value;
}

float plc_mem_get_real(uint16_t index)
{
    if (index >= PLC_MEM_REAL_COUNT) {
        return 0.0f;
    }

    return s_memReal[index];
}

PlcMemoryResult plc_mem_read_bool_range(uint16_t index, uint16_t count, bool* out_values)
{
    if (out_values == NULL) {
        return PLC_MEM_ERR_BAD_ARG;
    }

    PlcMemoryResult r = plc_mem_validate_range(PLC_MEM_TYPE_BOOL, index, count);
    if (r != PLC_MEM_OK) {
        return r;
    }

    for (uint16_t i = 0u; i < count; i++) {
        out_values[i] = s_memBool[index + i];
    }

    return PLC_MEM_OK;
}

PlcMemoryResult plc_mem_write_bool_range(uint16_t index, uint16_t count, const bool* values)
{
    if (values == NULL) {
        return PLC_MEM_ERR_BAD_ARG;
    }

    PlcMemoryResult r = plc_mem_validate_range(PLC_MEM_TYPE_BOOL, index, count);
    if (r != PLC_MEM_OK) {
        return r;
    }

    for (uint16_t i = 0u; i < count; i++) {
        s_memBool[index + i] = values[i];
    }

    return PLC_MEM_OK;
}

PlcMemoryResult plc_mem_read_int_range(uint16_t index, uint16_t count, int32_t* out_values)
{
    if (out_values == NULL) {
        return PLC_MEM_ERR_BAD_ARG;
    }

    PlcMemoryResult r = plc_mem_validate_range(PLC_MEM_TYPE_INT, index, count);
    if (r != PLC_MEM_OK) {
        return r;
    }

    for (uint16_t i = 0u; i < count; i++) {
        out_values[i] = s_memInt[index + i];
    }

    return PLC_MEM_OK;
}

PlcMemoryResult plc_mem_write_int_range(uint16_t index, uint16_t count, const int32_t* values)
{
    if (values == NULL) {
        return PLC_MEM_ERR_BAD_ARG;
    }

    PlcMemoryResult r = plc_mem_validate_range(PLC_MEM_TYPE_INT, index, count);
    if (r != PLC_MEM_OK) {
        return r;
    }

    for (uint16_t i = 0u; i < count; i++) {
        s_memInt[index + i] = values[i];
    }

    return PLC_MEM_OK;
}

PlcMemoryResult plc_mem_read_real_range(uint16_t index, uint16_t count, float* out_values)
{
    if (out_values == NULL) {
        return PLC_MEM_ERR_BAD_ARG;
    }

    PlcMemoryResult r = plc_mem_validate_range(PLC_MEM_TYPE_REAL, index, count);
    if (r != PLC_MEM_OK) {
        return r;
    }

    for (uint16_t i = 0u; i < count; i++) {
        out_values[i] = s_memReal[index + i];
    }

    return PLC_MEM_OK;
}

PlcMemoryResult plc_mem_write_real_range(uint16_t index, uint16_t count, const float* values)
{
    if (values == NULL) {
        return PLC_MEM_ERR_BAD_ARG;
    }

    PlcMemoryResult r = plc_mem_validate_range(PLC_MEM_TYPE_REAL, index, count);
    if (r != PLC_MEM_OK) {
        return r;
    }

    for (uint16_t i = 0u; i < count; i++) {
        s_memReal[index + i] = values[i];
    }

    return PLC_MEM_OK;
}

void plc_mem_reset_all(void)
{
    memset(s_memBool, 0, sizeof(s_memBool));
    memset(s_memInt,  0, sizeof(s_memInt));
    memset(s_memReal, 0, sizeof(s_memReal));
}

void plc_mem_init(void)
{
    plc_mem_reset_all();
}