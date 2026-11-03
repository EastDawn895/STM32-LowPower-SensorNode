#ifndef W25Q64_H
#define W25Q64_H

#include "stm32f1xx_hal.h" // Blue Pill 使用的是 F1 系列
#include <stdint.h>

void W25Q64_Init(void);
uint32_t W25Q64_ReadID(void);
void W25Q64_EraseSector(uint32_t addr);
void W25Q64_PageProgram(uint32_t addr, uint8_t* buf, uint16_t len);
void W25Q64_ReadData(uint32_t addr, uint8_t* buf, uint16_t len);
#endif