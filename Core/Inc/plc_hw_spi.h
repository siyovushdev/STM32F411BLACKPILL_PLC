#pragma once

#include <stdint.h>
#include <stdbool.h>

bool plc_hw_spi_init(void);
bool plc_hw_spi_read_di(uint16_t *di_bits);
bool plc_hw_spi_write_do(uint16_t do_bits);

uint16_t plc_hw_spi_get_di_total(void);
uint16_t plc_hw_spi_get_do_total(void);

/*
 * Общая SPI1-шина используется сразу несколькими микросхемами:
 * - MCP23S17 DI/DO: SPI mode 0
 * - AD7606: SPI mode 0 в текущей схеме чтения
 * - DAC8568: SPI mode 1
 *
 * Любая транзакция на SPI1 должна идти под этим lock-ом, иначе scan-задача PLC
 * может одновременно читать DI/AI и писать AO/DO, что приводит к битым SPI кадрам.
 */
void plc_hw_spi_lock(void);
void plc_hw_spi_unlock(void);

bool plc_hw_spi_configure_mode0(void); /* CPOL=0, CPHA=1EDGE */
bool plc_hw_spi_configure_mode1(void); /* CPOL=0, CPHA=2EDGE */