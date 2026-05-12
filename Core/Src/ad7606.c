#include "ad7606.h"

static inline void ad7606_cs_low(AD7606_Handle *d)  { HAL_GPIO_WritePin(d->CS_Port,  d->CS_Pin,  GPIO_PIN_RESET); }
static inline void ad7606_cs_high(AD7606_Handle *d) { HAL_GPIO_WritePin(d->CS_Port,  d->CS_Pin,  GPIO_PIN_SET);  }
static inline void ad7606_rst_low(AD7606_Handle *d) { HAL_GPIO_WritePin(d->RST_Port, d->RST_Pin, GPIO_PIN_RESET); }
static inline void ad7606_rst_high(AD7606_Handle *d){ HAL_GPIO_WritePin(d->RST_Port, d->RST_Pin, GPIO_PIN_SET);  }
static inline void ad7606_cva_low(AD7606_Handle *d) { HAL_GPIO_WritePin(d->CVA_Port, d->CVA_Pin, GPIO_PIN_RESET); }
static inline void ad7606_cva_high(AD7606_Handle *d){ HAL_GPIO_WritePin(d->CVA_Port, d->CVA_Pin, GPIO_PIN_SET);  }
static inline void ad7606_cvb_low(AD7606_Handle *d) { HAL_GPIO_WritePin(d->CVB_Port, d->CVB_Pin, GPIO_PIN_RESET); }
static inline void ad7606_cvb_high(AD7606_Handle *d){ HAL_GPIO_WritePin(d->CVB_Port, d->CVB_Pin, GPIO_PIN_SET);  }

static bool ad7606_is_valid(const AD7606_Handle *dev)
{
    return dev != NULL &&
           dev->hspi != NULL &&
           dev->CS_Port != NULL && dev->RST_Port != NULL && dev->BUSY_Port != NULL &&
           dev->CVA_Port != NULL && dev->CVB_Port != NULL;
}

HAL_StatusTypeDef AD7606_Init(AD7606_Handle *dev)
{
    if (!ad7606_is_valid(dev)) {
        return HAL_ERROR;
    }

    if (dev->t_conv_timeout_ms == 0u) dev->t_conv_timeout_ms = 10u;
    if (dev->spi_timeout_ms == 0u) dev->spi_timeout_ms = 10u;

    ad7606_cs_high(dev);
    ad7606_cva_high(dev);
    ad7606_cvb_high(dev);
    ad7606_rst_low(dev);

    ad7606_rst_high(dev);
    HAL_Delay(2u);
    ad7606_rst_low(dev);
    HAL_Delay(2u);

    return HAL_OK;
}

void AD7606_StartConversion(AD7606_Handle *dev)
{
    if (!ad7606_is_valid(dev)) {
        return;
    }

    ad7606_cva_low(dev);
    ad7606_cvb_low(dev);

    for (volatile uint32_t i = 0; i < 200u; i++) {
        __NOP();
    }

    ad7606_cva_high(dev);
    ad7606_cvb_high(dev);
}

HAL_StatusTypeDef AD7606_WaitBusyLow(AD7606_Handle *dev)
{
    if (!ad7606_is_valid(dev)) {
        return HAL_ERROR;
    }

    const uint32_t t0 = HAL_GetTick();

    while (AD7606_IsBusy(dev)) {
        if ((HAL_GetTick() - t0) > dev->t_conv_timeout_ms) {
            return HAL_TIMEOUT;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef AD7606_ReadRaw(AD7606_Handle *dev, int16_t out[AD7606_CH_COUNT])
{
    if (!ad7606_is_valid(dev) || out == NULL) {
        return HAL_ERROR;
    }

    uint8_t rx[AD7606_CH_COUNT * 2u] = {0};
    uint8_t tx[AD7606_CH_COUNT * 2u] = {0};

    ad7606_cs_low(dev);
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(dev->hspi, tx, rx, (uint16_t)sizeof(rx), dev->spi_timeout_ms);
    ad7606_cs_high(dev);

    if (st != HAL_OK) {
        return st;
    }

    for (uint8_t i = 0u; i < AD7606_CH_COUNT; i++) {
        const uint16_t w = ((uint16_t)rx[i * 2u] << 8u) | (uint16_t)rx[i * 2u + 1u];
        out[i] = (int16_t)w;
    }

    return HAL_OK;
}

HAL_StatusTypeDef AD7606_ReadSample(AD7606_Handle *dev, int16_t out[AD7606_CH_COUNT])
{
    if (!ad7606_is_valid(dev) || out == NULL) {
        return HAL_ERROR;
    }

    AD7606_StartConversion(dev);

    HAL_StatusTypeDef st = AD7606_WaitBusyLow(dev);
    if (st != HAL_OK) {
        return st;
    }

    return AD7606_ReadRaw(dev, out);
}
