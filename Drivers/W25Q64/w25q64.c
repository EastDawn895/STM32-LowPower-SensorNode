#include "w25q64.h"
#include "main.h" // 包含 CubeMX 生成的引脚宏定义 W25Q64_CS_GPIO_Port 等

extern SPI_HandleTypeDef hspi2;

/* 软件控制片选拉低拉高 */
#define CS_LOW()  HAL_GPIO_WritePin(W25Q64_CS_GPIO_Port, W25Q64_CS_Pin, GPIO_PIN_RESET)
#define CS_HIGH() HAL_GPIO_WritePin(W25Q64_CS_GPIO_Port, W25Q64_CS_Pin, GPIO_PIN_SET)

/* SPI 底层收发单字节 (全双工) */
static uint8_t SPI_SwapByte(uint8_t byte)
{
    uint8_t rx_data = 0;
    // 发送一个字节的同时，接收一个字节
    HAL_SPI_TransmitReceive(&hspi2, &byte, &rx_data, 1, 100);
    return rx_data;
}

void W25Q64_Init(void)
{
    CS_HIGH();     // 确保总线空闲，禁止误触发
    HAL_Delay(10); // 上电后给芯片一点稳定时间
}

uint32_t W25Q64_ReadID(void)
{
    uint32_t id = 0;

    CS_LOW(); // 1. 拉低片选，开始通信

    SPI_SwapByte(0x9F); // 2. 发送 0x9F (读取 JEDEC ID 的标准指令)

    // 3. 连续发送 3 个 0xFF 假数据(Dummy)来产生时钟，以此把芯片的数据“挤”回来
    id |= (SPI_SwapByte(0xFF) << 16); // 收到厂商 ID (Winbond 应为 0xEF)
    id |= (SPI_SwapByte(0xFF) << 8);  // 收到存储器类型 (应为 0x40)
    id |= SPI_SwapByte(0xFF);         // 收到容量大小 (应为 0x17)

    CS_HIGH(); // 4. 拉高片选，结束通信

    return id;
}

static void W25Q64_WriteEnable(void)
{

    CS_LOW();
    SPI_SwapByte(0x06);
    CS_HIGH();

}

static void W25Q64_WaitBusy(void)
{
    uint8_t status=0;
    CS_LOW();
    do
    {
        status = SPI_SwapByte(0xFF);
    }while ((status&0x01)  == 0x01);

    CS_HIGH();
}

void W25Q64_EraseSector(uint32_t addr)
{
    W25Q64_WriteEnable();
    CS_LOW();
    SPI_SwapByte(0x20);
    SPI_SwapByte((addr>>16)&0xFF);
    SPI_SwapByte((addr>>8)&0xFF);
    SPI_SwapByte(addr&0xFF);
    CS_HIGH();
    W25Q64_WaitBusy();
}


void W25Q64_PageProgram(uint32_t addr, uint8_t* buf, uint16_t len)
{
    W25Q64_WriteEnable();
    CS_LOW();
    SPI_SwapByte(0x02);
    SPI_SwapByte((addr>>16)&0xFF);
    SPI_SwapByte((addr>>8)&0xFF);
    SPI_SwapByte(addr&0xFF);
    for (uint16_t i=0; i<len; i++)
    {
        SPI_SwapByte(buf[i]);
    }
    CS_HIGH();
    W25Q64_WaitBusy();
}

void W25Q64_ReadData(uint32_t addr, uint8_t* buf, uint16_t len)
{
    W25Q64_WaitBusy();
    CS_LOW();
    SPI_SwapByte(0x03);
    SPI_SwapByte((addr>>16)&0xFF);
    SPI_SwapByte((addr>>8)&0xFF);
    SPI_SwapByte(addr&0xFF);
    for (uint16_t i=0; i<len; i++)
    {
        buf[i] = SPI_SwapByte(0xFF);
    }
    CS_HIGH();
}