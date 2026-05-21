#include "plc_hw_spi.h"
#include "mcp23s17.h"
#include "main.h"
#include "FreeRTOS.h"
#include "semphr.h"

extern SPI_HandleTypeDef hspi1;

static SemaphoreHandle_t s_spi1_mutex = NULL;

void plc_hw_spi_lock(void)
{
    if (s_spi1_mutex != NULL) {
        xSemaphoreTake(s_spi1_mutex, portMAX_DELAY);
    }
}

void plc_hw_spi_unlock(void)
{
    if (s_spi1_mutex != NULL) {
        xSemaphoreGive(s_spi1_mutex);
    }
}

static bool plc_hw_spi_apply_phase(uint32_t phase)
{
    if (hspi1.Init.CLKPhase == phase &&
        hspi1.Init.CLKPolarity == SPI_POLARITY_LOW) {
        return true;
    }

    if (HAL_SPI_DeInit(&hspi1) != HAL_OK) {
        return false;
    }

    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = phase;

    return HAL_SPI_Init(&hspi1) == HAL_OK;
}

bool plc_hw_spi_configure_mode0(void)
{
    return plc_hw_spi_apply_phase(SPI_PHASE_1EDGE);
}

bool plc_hw_spi_configure_mode1(void)
{
    return plc_hw_spi_apply_phase(SPI_PHASE_2EDGE);
}

static Mcp23s17 s_mcp_di;
static Mcp23s17 s_mcp_do;

static uint16_t s_last_di = 0;
static uint16_t s_last_do = 0;

bool plc_hw_spi_init(void)
{
    if (s_spi1_mutex == NULL) {
        s_spi1_mutex = xSemaphoreCreateMutex();
        if (s_spi1_mutex == NULL) {
            return false;
        }
    }

    plc_hw_spi_lock();

    if (!plc_hw_spi_configure_mode0()) {
        plc_hw_spi_unlock();
        return false;
    }

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

    plc_hw_spi_unlock();

    return plc_hw_spi_write_do(0x0000u);
}

bool plc_hw_spi_read_di(uint16_t *di_bits)
{
    if (di_bits == NULL) {
        return false;
    }

    plc_hw_spi_lock();

    if (!plc_hw_spi_configure_mode0()) {
        plc_hw_spi_unlock();
        return false;
    }

    uint16_t value = 0;
    bool ok = mcp23s17_read_gpio16(&s_mcp_di, &value);

    if (ok) {
        s_last_di = value;
        *di_bits = value;
    }

    plc_hw_spi_unlock();
    return ok;
}

bool plc_hw_spi_write_do(uint16_t do_bits)
{
    plc_hw_spi_lock();

    if (!plc_hw_spi_configure_mode0()) {
        plc_hw_spi_unlock();
        return false;
    }

    bool ok = mcp23s17_write_gpio16(&s_mcp_do, do_bits);

    if (ok) {
        s_last_do = do_bits;
    }

    plc_hw_spi_unlock();
    return ok;
}

uint16_t plc_hw_spi_get_di_total(void)
{
    return 16u;
}

uint16_t plc_hw_spi_get_do_total(void)
{
    return 16u;
}