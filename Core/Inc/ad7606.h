#pragma once

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AD7606_CH_COUNT 8u

typedef struct {
    SPI_HandleTypeDef *hspi;

    GPIO_TypeDef *CS_Port;
    uint16_t CS_Pin;
    GPIO_TypeDef *RST_Port;
    uint16_t RST_Pin;
    GPIO_TypeDef *BUSY_Port;
    uint16_t BUSY_Pin;
    GPIO_TypeDef *CVA_Port;
    uint16_t CVA_Pin;
    GPIO_TypeDef *CVB_Port;
    uint16_t CVB_Pin;

    uint32_t t_conv_timeout_ms;
    uint32_t spi_timeout_ms;
} AD7606_Handle;

HAL_StatusTypeDef AD7606_Init(AD7606_Handle *dev);

void AD7606_StartConversion(AD7606_Handle *dev);

HAL_StatusTypeDef AD7606_WaitBusyLow(AD7606_Handle *dev);

HAL_StatusTypeDef AD7606_ReadRaw(AD7606_Handle *dev, int16_t out[AD7606_CH_COUNT]);

HAL_StatusTypeDef AD7606_ReadSample(AD7606_Handle *dev, int16_t out[AD7606_CH_COUNT]);

static inline bool AD7606_IsBusy(AD7606_Handle *dev) {
    return dev != NULL && HAL_GPIO_ReadPin(dev->BUSY_Port, dev->BUSY_Pin) == GPIO_PIN_SET;
}

#ifdef __cplusplus
}
#endif
