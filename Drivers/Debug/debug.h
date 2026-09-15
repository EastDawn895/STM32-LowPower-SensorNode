#ifndef __DEBUG_H
#define __DEBUG_H

#include "stm32f1xx_hal.h"

void Debug_Init(UART_HandleTypeDef *huart);
void Debug_Print(const char *msg);

#endif /* __DEBUG_H */