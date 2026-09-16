#ifndef DEBUG_H
#define DEBUG_H

#include "stm32f1xx_hal.h"

void Debug_Init(UART_HandleTypeDef *huart);
void Debug_Print(const char *msg);
void Debug_PrintHex(uint8_t val);
void Debug_PrintInt(int32_t val);

#endif /* DEBUG_H */