#include "mcp23s17.h"

#define MCP23S17_OPCODE_WRITE_BASE 0x40u
#define MCP23S17_OPCODE_READ_BASE  0x41u

#define MCP23S17_IODIRA  0x00u
#define MCP23S17_IODIRB  0x01u
#define MCP23S17_IOCONA  0x0Au
#define MCP23S17_GPPUA   0x0Cu
#define MCP23S17_GPPUB   0x0Du
#define MCP23S17_GPIOA   0x12u
#define MCP23S17_GPIOB   0x13u
#define MCP23S17_OLATA   0x14u
#define MCP23S17_OLATB   0x15u

#define MCP_TIMEOUT_MS   10u

static inline void mcp_cs_low(Mcp23s17 *dev)
{
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
}

static inline void mcp_cs_high(Mcp23s17 *dev)
{
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

static uint8_t mcp_opcode_write(const Mcp23s17 *dev)
{
    return (uint8_t)(MCP23S17_OPCODE_WRITE_BASE | ((dev->hw_addr & 0x07u) << 1u));
}

static uint8_t mcp_opcode_read(const Mcp23s17 *dev)
{
    return (uint8_t)(MCP23S17_OPCODE_READ_BASE | ((dev->hw_addr & 0x07u) << 1u));
}

static bool mcp_write_reg(Mcp23s17 *dev, uint8_t reg, uint8_t value)
{
    uint8_t tx[3] = {
            mcp_opcode_write(dev),
            reg,
            value
    };

    mcp_cs_low(dev);
    HAL_StatusTypeDef st = HAL_SPI_Transmit(dev->hspi, tx, sizeof(tx), MCP_TIMEOUT_MS);
    mcp_cs_high(dev);

    return st == HAL_OK;
}

static bool mcp_read_reg(Mcp23s17 *dev, uint8_t reg, uint8_t *value)
{
    uint8_t tx[3] = {
            mcp_opcode_read(dev),
            reg,
            0x00u
    };

    uint8_t rx[3] = {0};

    mcp_cs_low(dev);
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(dev->hspi, tx, rx, sizeof(tx), MCP_TIMEOUT_MS);
    mcp_cs_high(dev);

    if (st != HAL_OK) {
        return false;
    }

    *value = rx[2];
    return true;
}

bool mcp23s17_init_input(Mcp23s17 *dev)
{
    if (dev == NULL || dev->hspi == NULL) {
        return false;
    }

    mcp_cs_high(dev);

    return
            mcp_write_reg(dev, MCP23S17_IOCONA, 0x08u) &&   // HAEN=1
            mcp_write_reg(dev, MCP23S17_IODIRA, 0xFFu) &&
            mcp_write_reg(dev, MCP23S17_IODIRB, 0xFFu) &&
            mcp_write_reg(dev, MCP23S17_GPPUA,  0xFFu) &&
            mcp_write_reg(dev, MCP23S17_GPPUB,  0xFFu);
}

bool mcp23s17_init_output(Mcp23s17 *dev)
{
    if (dev == NULL || dev->hspi == NULL) {
        return false;
    }

    mcp_cs_high(dev);

    return
            mcp_write_reg(dev, MCP23S17_IOCONA, 0x08u) &&   // HAEN=1
            mcp_write_reg(dev, MCP23S17_IODIRA, 0x00u) &&
            mcp_write_reg(dev, MCP23S17_IODIRB, 0x00u) &&
            mcp_write_reg(dev, MCP23S17_OLATA,  0x00u) &&
            mcp_write_reg(dev, MCP23S17_OLATB,  0x00u);
}

bool mcp23s17_read_gpio16(Mcp23s17 *dev, uint16_t *value)
{
    if (dev == NULL || value == NULL) {
        return false;
    }

    uint8_t a = 0;
    uint8_t b = 0;

    if (!mcp_read_reg(dev, MCP23S17_GPIOA, &a)) {
        return false;
    }

    if (!mcp_read_reg(dev, MCP23S17_GPIOB, &b)) {
        return false;
    }

    *value = (uint16_t)((uint16_t)a | ((uint16_t)b << 8u));
    return true;
}

bool mcp23s17_write_gpio16(Mcp23s17 *dev, uint16_t value)
{
    if (dev == NULL) {
        return false;
    }

    return
            mcp_write_reg(dev, MCP23S17_OLATA, (uint8_t)(value & 0xFFu)) &&
            mcp_write_reg(dev, MCP23S17_OLATB, (uint8_t)((value >> 8u) & 0xFFu));
}