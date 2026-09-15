#include "debug.h"
#include <string.h>

static UART_HandleTypeDef *s_debug_uart = NULL;

void Debug_Init(UART_HandleTypeDef *huart)
{
    s_debug_uart = huart;
}

void Debug_Print(const char *msg)
{
    if ((s_debug_uart == NULL) || (msg == NULL)) {
        return;
    }
    HAL_UART_Transmit(s_debug_uart, (uint8_t *)msg, strlen(msg), 1000);
}
