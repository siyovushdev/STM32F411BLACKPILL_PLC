#pragma once

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

HAL_StatusTypeDef DAC8568_Init(void);
HAL_StatusTypeDef DAC8568_SetCh_Volts(uint8_t ch, float volts);
HAL_StatusTypeDef DAC8568_SetCh_Code(uint8_t ch, uint16_t code16);
HAL_StatusTypeDef DAC8568_WriteAll(uint16_t code16);
HAL_StatusTypeDef DAC8568_WriteCh(uint8_t ch, uint16_t code16);
HAL_StatusTypeDef DAC8568_UpdateCh(uint8_t ch);
HAL_StatusTypeDef DAC8568_ResetOutputsToZeroVolt(void);

#ifdef __cplusplus
}
#endif
