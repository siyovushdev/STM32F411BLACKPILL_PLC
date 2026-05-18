#pragma once

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    uint8_t hw_addr;
} Mcp23s17;

bool mcp23s17_init_input(Mcp23s17 *dev);
bool mcp23s17_init_output(Mcp23s17 *dev);
bool mcp23s17_read_gpio16(Mcp23s17 *dev, uint16_t *value);
bool mcp23s17_write_gpio16(Mcp23s17 *dev, uint16_t value);