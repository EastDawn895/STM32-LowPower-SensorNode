#include "w25q64.h"
#include "main.h"
extern SPI_HandleTypeDef hspi2;
static uint8_t ready, active;
static uint32_t started, budget;
#define CS(x) HAL_GPIO_WritePin(W25Q64_CS_GPIO_Port,W25Q64_CS_Pin,(x))
#define BUS_MS 10u
/* One deadline covers command and payload; every exit releases CS. */
static HAL_StatusTypeDef transaction(const uint8_t* cmd, uint16_t n, uint8_t* data, uint16_t len, uint8_t read)
{
    uint32_t start = HAL_GetTick();
    CS(GPIO_PIN_RESET);
    HAL_StatusTypeDef rc = HAL_SPI_Transmit(&hspi2, (uint8_t*)cmd, n,BUS_MS);
    uint32_t elapsed = HAL_GetTick() - start;
    if (rc == HAL_OK && len)
    {
        if (elapsed >= BUS_MS) rc = HAL_TIMEOUT;
        else if (read) rc = HAL_SPI_Receive(&hspi2, data, len,BUS_MS - elapsed);
        else rc = HAL_SPI_Transmit(&hspi2, data, len,BUS_MS - elapsed);
    }
    CS(GPIO_PIN_SET);
    if (rc == HAL_OK && HAL_GetTick() - start >= BUS_MS) rc = HAL_TIMEOUT;
    return rc;
}

static HAL_StatusTypeDef status(uint8_t* value)
{
    const uint8_t cmd = 0x05;
    HAL_StatusTypeDef rc = transaction(&cmd, 1, value, 1, 1);
    return rc == HAL_OK && *value == 0xFFu ? HAL_ERROR : rc;
}

HAL_StatusTypeDef W25Q64_ReadIDChecked(uint32_t* id)
{
    if (!id) return HAL_ERROR;
    uint8_t b[3], cmd = 0x9F;
    HAL_StatusTypeDef rc = transaction(&cmd, 1, b, 3, 1);
    *id = rc == HAL_OK ? ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2] : 0u;
    return rc;
}

uint32_t W25Q64_ReadID(void)
{
    uint32_t id = 0;
    (void)W25Q64_ReadIDChecked(&id);
    return id;
}

HAL_StatusTypeDef W25Q64_Poll(void)
{
    if (!ready) return HAL_ERROR;
    uint8_t sr = 0;
    HAL_StatusTypeDef rc = status(&sr);
    if (rc != HAL_OK)
    {
        ready = 0;
        return rc;
    }
    if (!(sr & 1u))
    {
        active = 0;
        return HAL_OK;
    }
    if (active && HAL_GetTick() - started >= budget)
    {
        ready = 0;
        return HAL_TIMEOUT;
    }
    return HAL_BUSY;
}

static HAL_StatusTypeDef wait(uint32_t limit)
{
    uint32_t start = HAL_GetTick();
    HAL_StatusTypeDef rc;
    do
    {
        rc = W25Q64_Poll();
        if (rc != HAL_BUSY) return rc;
        if (HAL_GetTick() - start >= limit)
        {
            ready = 0;
            return HAL_TIMEOUT;
        }
        HAL_Delay(1);
    }
    while (1);
}

HAL_StatusTypeDef W25Q64_Init(void)
{
    ready = 1;
    active = 0;
    CS(GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_StatusTypeDef rc = wait(500u);
    uint32_t id = 0;
    if (rc == HAL_OK) rc = W25Q64_ReadIDChecked(&id);
    /* Supported board part: Winbond W25Q64, 3.3 V, JEDEC EF4017. */
    if (rc == HAL_OK && id != 0xEF4017u) rc = HAL_ERROR;
    ready = (uint8_t)(rc == HAL_OK);
    return rc;
}

static HAL_StatusTypeDef write_enable(void)
{
    HAL_StatusTypeDef rc = W25Q64_Poll();
    if (rc != HAL_OK) return rc;
    uint8_t cmd = 0x06, sr = 0;
    rc = transaction(&cmd, 1, 0, 0, 0);
    if (rc == HAL_OK) rc = status(&sr);
    if (rc == HAL_OK && !(sr & 2u)) rc = HAL_ERROR;
    return rc;
}

static void address(uint8_t* cmd, uint8_t op, uint32_t addr)
{
    cmd[0] = op;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)addr;
}

HAL_StatusTypeDef W25Q64_ReadData(uint32_t addr, uint8_t* buf, uint16_t len)
{
    if (!buf || !len || addr >= W25Q64_CAPACITY || len > W25Q64_CAPACITY - addr) return HAL_ERROR;
    HAL_StatusTypeDef rc = W25Q64_Poll();
    uint8_t cmd[4];
    if (rc != HAL_OK) return rc;
    address(cmd, 0x03, addr);
    return transaction(cmd, 4, buf, len, 1);
}

HAL_StatusTypeDef W25Q64_BeginErase(uint32_t addr)
{
    if (addr >= W25Q64_CAPACITY || addr % 4096u) return HAL_ERROR;
    HAL_StatusTypeDef rc = write_enable();
    uint8_t cmd[4];
    if (rc != HAL_OK) return rc;
    address(cmd, 0x20, addr);
    rc = transaction(cmd, 4, 0, 0, 0);
    if (rc == HAL_OK)
    {
        active = 1;
        started = HAL_GetTick();
        budget = 500u;
    }
    return rc;
}

HAL_StatusTypeDef W25Q64_BeginProgram(uint32_t addr, const uint8_t* buf, uint16_t len)
{
    if (!buf || !len || len > 256u || addr >= W25Q64_CAPACITY || len > W25Q64_CAPACITY - addr || (addr % 256u) + len >
        256u) return HAL_ERROR;
    HAL_StatusTypeDef rc = write_enable();
    uint8_t cmd[4];
    if (rc != HAL_OK) return rc;
    address(cmd, 0x02, addr);
    rc = transaction(cmd, 4, (uint8_t*)buf, len, 0);
    if (rc == HAL_OK)
    {
        active = 1;
        started = HAL_GetTick();
        budget = 10u;
    }
    return rc;
}

HAL_StatusTypeDef W25Q64_EraseSector(uint32_t addr)
{
    HAL_StatusTypeDef rc = W25Q64_BeginErase(addr);
    return rc == HAL_OK ? wait(500u) : rc;
}

HAL_StatusTypeDef W25Q64_PageProgram(uint32_t addr, const uint8_t* buf, uint16_t len)
{
    HAL_StatusTypeDef rc = W25Q64_BeginProgram(addr, buf, len);
    return rc == HAL_OK ? wait(10u) : rc;
}
