#include "debug.h"
#include <string.h>

static UART_HandleTypeDef *s_debug_uart = NULL;

/* 打印定点数：val 隐含 decimals 位小数，例如 (105, 2) 打印 "1.05" */
void Debug_PrintFixed(int32_t val, int decimals)
{
    int32_t div = 1;
    int i;

    for (i = 0; i < decimals; i++) {
        div *= 10;
    }

    if (val < 0) {          /* 先处理负号，之后全按正数算 */
        Debug_Print("-");
        val = -val;
    }

    Debug_PrintInt(val / div);
    Debug_Print(".");

    /* 补前导零：1.05 的 "05" 不能只打 "5" */
    for (i = div / 10; i > 0 && (val % div) < i; i /= 10) {
        Debug_Print("0");
    }
    Debug_PrintInt(val % div);
}

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

void Debug_PrintInt(int32_t val)
{
    char buf[12];
    char digits[12];
    int i = 0, j = 0;
    uint32_t u;

    if (val < 0) {
        buf[i++] = '-';
        u = (uint32_t)(-val);
    } else {
        u = (uint32_t)val;
    }

    do {                                    /* 低位先存进 digits */
        digits[j++] = (char)('0' + (u % 10));
        u /= 10;
    } while (u > 0);

    while (j > 0) {                         /* 反转写回 buf */
        buf[i++] = digits[--j];
    }
    buf[i] = '\0';                          /* 结尾必须补 \0 */

    Debug_Print(buf);
}


