#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "friendly_plc/plc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLC_MEM_TYPE_BOOL = 0,
    PLC_MEM_TYPE_INT  = 1,
    PLC_MEM_TYPE_REAL = 2
} PlcMemoryType;

typedef enum {
    PLC_MEM_OK = 0,
    PLC_MEM_ERR_BAD_TYPE,
    PLC_MEM_ERR_BAD_INDEX,
    PLC_MEM_ERR_BAD_RANGE,
    PLC_MEM_ERR_BAD_ARG
} PlcMemoryResult;

typedef struct {
    uint16_t bool_count;
    uint16_t int_count;
    uint16_t real_count;

    uint16_t bool_size;
    uint16_t int_size;
    uint16_t real_size;
} PlcMemoryInfo;

void plc_mem_init(void);
void plc_mem_reset_all(void);

void plc_mem_get_info(PlcMemoryInfo* out);
PlcMemoryResult plc_mem_get_count(PlcMemoryType type, uint16_t* out_count);
PlcMemoryResult plc_mem_validate_range(PlcMemoryType type, uint16_t index, uint16_t count);

void plc_mem_set_bool(uint16_t index, bool value);
bool plc_mem_get_bool(uint16_t index);

void plc_mem_set_int(uint16_t index, int32_t value);
int32_t plc_mem_get_int(uint16_t index);

void plc_mem_set_real(uint16_t index, float value);
float plc_mem_get_real(uint16_t index);

PlcMemoryResult plc_mem_read_bool_range(uint16_t index, uint16_t count, bool* out_values);
PlcMemoryResult plc_mem_write_bool_range(uint16_t index, uint16_t count, const bool* values);

PlcMemoryResult plc_mem_read_int_range(uint16_t index, uint16_t count, int32_t* out_values);
PlcMemoryResult plc_mem_write_int_range(uint16_t index, uint16_t count, const int32_t* values);

PlcMemoryResult plc_mem_read_real_range(uint16_t index, uint16_t count, float* out_values);
PlcMemoryResult plc_mem_write_real_range(uint16_t index, uint16_t count, const float* values);

#ifdef __cplusplus
}
#endif