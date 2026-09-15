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

void Debug_PrintHex(uint8_t val)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[5] = "0x00";
    buf[2] = hex[val >> 4];
    buf[3] = hex[val & 0x0F];
    Debug_Print(buf);
}


