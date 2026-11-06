#include <assert.h>
#include <string.h>
#include <limits.h>
#include "debug.h"
static char out[300]; static uint32_t tick; static HAL_StatusTypeDef rc;
uint32_t HAL_GetTick(void) { return tick; }
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *u,uint8_t *s,uint16_t n,uint32_t t) {
    (void)u; assert(t<=20u); if(rc!=HAL_OK) return rc;
    assert(strlen(out)+n<sizeof(out)); strncat(out,(char *)s,n); return HAL_OK;
}
static void fixed(int32_t n,int d,const char *expected) {
    out[0]=0; Debug_BeginFrame(); Debug_PrintFixed(n,d); Debug_EndFrame(); assert(strcmp(out,expected)==0);
}
int main(void) {
    UART_HandleTypeDef u; Debug_Init(&u);
    fixed(0,2,"0.00");fixed(100,2,"1.00");fixed(-105,2,"-1.05");fixed(1,2,"0.01");
    fixed(INT32_MIN,0,"-2147483648");fixed(INT32_MAX,9,"2.147483647");fixed(-1,9,"-0.000000001");
    fixed(0,0,"0");fixed(0,9,"0.000000000");
    out[0]=0;Debug_BeginFrame();Debug_PrintUInt(UINT32_MAX);assert(strcmp(out,"4294967295")==0);
    out[0]=0;Debug_BeginFrame();Debug_PrintInt(INT32_MIN);assert(strcmp(out,"-2147483648")==0);
    Debug_BeginFrame();Debug_PrintFixed(1,10);assert(Debug_GetStatus()==HAL_ERROR);
    Debug_BeginFrame();tick+=20;Debug_Print("x");assert(Debug_GetStatus()==HAL_TIMEOUT);
    Debug_BeginFrame();rc=HAL_ERROR;Debug_Print("x");assert(Debug_GetStatus()==HAL_ERROR);
    return 0;
}
