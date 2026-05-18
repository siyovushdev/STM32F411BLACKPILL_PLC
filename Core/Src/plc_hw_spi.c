#include "plc_hw_spi.h"
#include "mcp23s17.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

static Mcp23s17 s_mcp_di;
static Mcp23s17 s_mcp_do;

static uint16_t s_last_di = 0;
static uint16_t s_last_do = 0;

bool plc_hw_spi_init(void)
{
    HAL_GPIO_WritePin(MCP_DI_CS_GPIO_Port, MCP_DI_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MCP_DO_CS_GPIO_Port, MCP_DO_CS_Pin, GPIO_PIN_SET);

    s_mcp_di.hspi = &hspi1;
    s_mcp_di.cs_port = MCP_DI_CS_GPIO_Port;
    s_mcp_di.cs_pin = MCP_DI_CS_Pin;
    s_mcp_di.hw_addr = 0;

    s_mcp_do.hspi = &hspi1;
    s_mcp_do.cs_port = MCP_DO_CS_GPIO_Port;
    s_mcp_do.cs_pin = MCP_DO_CS_Pin;
    s_mcp_do.hw_addr = 0;

    if (!mcp23s17_init_input(&s_mcp_di)) {
        return false;
    }

    if (!mcp23s17_init_output(&s_mcp_do)) {
        return false;
    }

    s_last_di = 0;
    s_last_do = 0;

    return plc_hw_spi_write_do(0x0000u);
}

bool plc_hw_spi_read_di(uint16_t *di_bits)
{
    if (di_bits == NULL) {
        return false;
    }

    uint16_t value = 0;
    if (!mcp23s17_read_gpio16(&s_mcp_di, &value)) {
        return false;
    }

    s_last_di = value;
    *di_bits = value;
    return true;
}

bool plc_hw_spi_write_do(uint16_t do_bits)
{
    if (!mcp23s17_write_gpio16(&s_mcp_do, do_bits)) {
        return false;
    }

    s_last_do = do_bits;
    return true;
}

uint16_t plc_hw_spi_get_di_total(void)
{
    return 16u;
}

uint16_t plc_hw_spi_get_do_total(void)
{
    return 16u;
}