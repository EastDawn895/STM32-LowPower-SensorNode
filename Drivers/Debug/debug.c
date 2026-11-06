#include "debug.h"
#include <stddef.h>

static UART_HandleTypeDef *s_uart;
static HAL_StatusTypeDef s_status;
static uint32_t s_start;
static uint8_t s_frame;
#define DEBUG_BUDGET_MS 20u

void Debug_Init(UART_HandleTypeDef *uart) { s_uart=uart; s_status=HAL_OK; s_frame=0; }
void Debug_BeginFrame(void) { s_start=HAL_GetTick(); s_status=HAL_OK; s_frame=1; }
void Debug_EndFrame(void) { s_frame=0; }
HAL_StatusTypeDef Debug_GetStatus(void) { return s_status; }
void Debug_Print(const char *msg)
{
    if (s_status!=HAL_OK) return;
    if (!s_uart || !msg) { s_status=HAL_ERROR; return; }
    uint16_t n=0;
    while (n<256u && msg[n]) n++;
    if (n==256u) { s_status=HAL_ERROR; return; }
    uint32_t elapsed=s_frame ? HAL_GetTick()-s_start : 0u;
    if (elapsed>=DEBUG_BUDGET_MS) { s_status=HAL_TIMEOUT; return; }
    if (n) s_status=HAL_UART_Transmit(s_uart,(uint8_t *)msg,n,DEBUG_BUDGET_MS-elapsed);
}
void Debug_PrintUInt(uint32_t val)
{
    char digits[11]; unsigned i=10; digits[i]='\0';
    do { digits[--i]=(char)('0'+val%10u); val/=10u; } while(val);
    Debug_Print(&digits[i]);
}
void Debug_PrintInt(int32_t val)
{
    uint32_t magnitude=(uint32_t)val;
    if (val<0) { Debug_Print("-"); magnitude=0u-magnitude; }
    Debug_PrintUInt(magnitude);
}
void Debug_PrintFixed(int32_t val,int decimals)
{
    if (decimals<0 || decimals>9) { s_status=HAL_ERROR; return; }
    uint32_t magnitude=(uint32_t)val, div=1;
    if (val<0) { Debug_Print("-"); magnitude=0u-magnitude; }
    for(int i=0;i<decimals;i++) div*=10u;
    Debug_PrintUInt(magnitude/div);
    if (!decimals) return;
    Debug_Print(".");
    for(uint32_t place=div/10u;place;place/=10u) {
        char c[2]={(char)('0'+(magnitude/place)%10u),'\0'};
        Debug_Print(c);
    }
}
void Debug_PrintHex(uint8_t val)
{
    static const char hex[]="0123456789ABCDEF";
    char b[5]={'0','x',hex[val>>4],hex[val&15u],'\0'}; Debug_Print(b);
}
