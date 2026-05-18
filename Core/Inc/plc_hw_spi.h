#pragma once

#include <stdint.h>
#include <stdbool.h>

bool plc_hw_spi_init(void);
bool plc_hw_spi_read_di(uint16_t *di_bits);
bool plc_hw_spi_write_do(uint16_t do_bits);

uint16_t plc_hw_spi_get_di_total(void);
uint16_t plc_hw_spi_get_do_total(void);