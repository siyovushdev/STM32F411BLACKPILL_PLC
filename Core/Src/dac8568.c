#include "dac8568.h"
#include <stdbool.h>
#include "plc_hw_spi.h"

extern SPI_HandleTypeDef hspi1;


static inline void dac_sync_high(void) { HAL_GPIO_WritePin(DAC_SYNC_GPIO_Port, DAC_SYNC_Pin, GPIO_PIN_SET); }

static inline void dac_sync_low(void) { HAL_GPIO_WritePin(DAC_SYNC_GPIO_Port, DAC_SYNC_Pin, GPIO_PIN_RESET); }

static inline void dac_clr_high(void) { HAL_GPIO_WritePin(DAC_CLR_GPIO_Port, DAC_CLR_Pin, GPIO_PIN_SET); }

static inline void dac_clr_low(void) { HAL_GPIO_WritePin(DAC_CLR_GPIO_Port, DAC_CLR_Pin, GPIO_PIN_RESET); }

#ifdef DAC_LDAC_GPIO_Port

static inline void dac_ldac_high(void) { HAL_GPIO_WritePin(DAC_LDAC_GPIO_Port, DAC_LDAC_Pin, GPIO_PIN_SET); }

#else
static inline void dac_ldac_high(void) {}
#endif

static inline uint32_t dac_make_word(uint8_t ctrl, uint8_t addr, uint16_t data, uint8_t feature) {
    return ((uint32_t) (ctrl & 0x0Fu) << 24u) |
           ((uint32_t) (addr & 0x0Fu) << 20u) |
           ((uint32_t) data << 4u) |
           ((uint32_t) (feature & 0x0Fu));
}

static HAL_StatusTypeDef dac_tx32_unlocked(uint32_t w) {
    uint8_t b[4] = {
            (uint8_t) (w >> 24u),
            (uint8_t) (w >> 16u),
            (uint8_t) (w >> 8u),
            (uint8_t) (w),
    };

    dac_sync_low();
    HAL_StatusTypeDef st = HAL_SPI_Transmit(&hspi1, b, 4u, 50u);
    dac_sync_high();

    return st;
}

static HAL_StatusTypeDef dac_sw_reset_unlocked(void) {
    return dac_tx32_unlocked(dac_make_word(0x7u, 0x0u, 0x0000u, 0x0u));
}

static HAL_StatusTypeDef dac_internal_ref_always_on_unlocked(void) {
    return dac_tx32_unlocked(0x090A0000u);
}

static HAL_StatusTypeDef dac_set_bipolar_10v_unlocked(void) {
    return dac_tx32_unlocked(dac_make_word(0x8u, 0x8u, 0x0005u, 0x0u));
}

static HAL_StatusTypeDef dac_power_up_all_unlocked(void) {
    return dac_tx32_unlocked(dac_make_word(0x4u, 0x0u, 0x00FFu, 0x0u));
}

static HAL_StatusTypeDef dac_write_update_ch_unlocked(uint8_t ch, uint16_t code16) {
    return dac_tx32_unlocked(dac_make_word(0x3u, ch & 0x0Fu, code16, 0x0u));
}

HAL_StatusTypeDef DAC8568_WriteCh(uint8_t ch, uint16_t code16) {
    if (ch > 7u) return HAL_ERROR;

    plc_hw_spi_lock();

    if (!plc_hw_spi_configure_mode1()) {
        plc_hw_spi_unlock();
        return HAL_ERROR;
    }
    HAL_StatusTypeDef st = dac_tx32_unlocked(dac_make_word(0x0u, ch & 0x0Fu, code16, 0x0u));
    plc_hw_spi_unlock();

    return st;
}

HAL_StatusTypeDef DAC8568_UpdateCh(uint8_t ch) {
    if (ch > 7u) return HAL_ERROR;

    plc_hw_spi_lock();

    if (!plc_hw_spi_configure_mode1()) {
        plc_hw_spi_unlock();
        return HAL_ERROR;
    }
    HAL_StatusTypeDef st = dac_tx32_unlocked(dac_make_word(0x1u, ch & 0x0Fu, 0x0000u, 0x0u));
    plc_hw_spi_unlock();

    return st;
}

HAL_StatusTypeDef DAC8568_WriteAll(uint16_t code16) {
    plc_hw_spi_lock();

    if (!plc_hw_spi_configure_mode1()) {
        plc_hw_spi_unlock();
        return HAL_ERROR;
    }
    HAL_StatusTypeDef st = dac_tx32_unlocked(dac_make_word(0x3u, 0xFu, code16, 0x0u));
    plc_hw_spi_unlock();

    return st;
}

HAL_StatusTypeDef DAC8568_SetCh_Code(uint8_t ch, uint16_t code16) {
    if (ch > 7u) return HAL_ERROR;

    plc_hw_spi_lock();

    if (!plc_hw_spi_configure_mode1()) {
        plc_hw_spi_unlock();
        return HAL_ERROR;
    }

    HAL_StatusTypeDef st = dac_set_bipolar_10v_unlocked();
    if (st == HAL_OK) {
        HAL_Delay(2u);
        st = dac_write_update_ch_unlocked(ch, code16);
    }

    plc_hw_spi_unlock();
    return st;
}

HAL_StatusTypeDef DAC8568_SetCh_Volts(uint8_t ch, float volts) {
    if (ch > 7u) return HAL_ERROR;

    if (volts < -10.0f) volts = -10.0f;
    if (volts > 10.0f) volts = 10.0f;

    uint32_t code = (uint32_t) ((volts + 10.0f) * 3276.75f + 0.5f);
    if (code > 65535u) code = 65535u;

    return DAC8568_SetCh_Code(ch, (uint16_t) code);
}

HAL_StatusTypeDef DAC8568_ResetOutputsToZeroVolt(void) {
    dac_clr_high();
    return DAC8568_WriteAll(0x8000u);
}

HAL_StatusTypeDef DAC8568_Init(void) {
    HAL_Delay(100u);

    dac_sync_high();
    dac_clr_high();
    dac_ldac_high();
    HAL_Delay(2u);

    plc_hw_spi_lock();

    if (!plc_hw_spi_configure_mode1()) {
        plc_hw_spi_unlock();
        return HAL_ERROR;
    }

    HAL_StatusTypeDef st = dac_sw_reset_unlocked();
    if (st != HAL_OK) {
        plc_hw_spi_unlock();
        return st;
    }
    HAL_Delay(10u);

    st = dac_internal_ref_always_on_unlocked();
    if (st != HAL_OK) {
        plc_hw_spi_unlock();
        return st;
    }
    HAL_Delay(50u);

    st = dac_set_bipolar_10v_unlocked();
    if (st != HAL_OK) {
        plc_hw_spi_unlock();
        return st;
    }
    HAL_Delay(20u);

    st = dac_power_up_all_unlocked();
    if (st != HAL_OK) {
        plc_hw_spi_unlock();
        return st;
    }
    HAL_Delay(5u);

    st = dac_write_update_ch_unlocked(0xFu, 0x8000u);

    plc_hw_spi_unlock();
    return st;
}
