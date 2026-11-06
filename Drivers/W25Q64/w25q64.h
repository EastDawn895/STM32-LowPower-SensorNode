#ifndef W25Q64_H
#define W25Q64_H
#include "stm32f1xx_hal.h"
/* Main-loop only, SysTick enabled. Erase requires 4 KiB alignment;
 * PageProgram rejects zero length, NULL, overflow and crossing a 256-byte page. */
#define W25Q64_CAPACITY (8u*1024u*1024u)
HAL_StatusTypeDef W25Q64_Init(void);
HAL_StatusTypeDef W25Q64_ReadIDChecked(uint32_t* id);
uint32_t W25Q64_ReadID(void); /* Compatibility wrapper: 0 on failure. */
HAL_StatusTypeDef W25Q64_ReadData(uint32_t addr, uint8_t* buf, uint16_t len);
HAL_StatusTypeDef W25Q64_EraseSector(uint32_t addr);
HAL_StatusTypeDef W25Q64_PageProgram(uint32_t addr, const uint8_t* buf, uint16_t len);
/* Begin/Poll never wait for the internal program/erase cycle. Main context only. */
HAL_StatusTypeDef W25Q64_BeginErase(uint32_t addr);
HAL_StatusTypeDef W25Q64_BeginProgram(uint32_t addr, const uint8_t* buf, uint16_t len);
HAL_StatusTypeDef W25Q64_Poll(void);
#endif
