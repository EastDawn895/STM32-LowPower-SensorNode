#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "monitor.h"
#include "debug.h"
#include "oled.h"
#include "sensor_state.h"
#include "motion_state.h"
#include "light_state.h"
#include "data_logger.h"
static char out[512],rows[4][17];static uint32_t tick=100;
uint32_t HAL_GetTick(void){return tick;}
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *h,uint8_t *b,uint16_t n,uint32_t t){(void)h;(void)t;assert(strlen(out)+n<sizeof out);strncat(out,(char *)b,n);return HAL_OK;}
void OLED_Init(void){}
HAL_StatusTypeDef OLED_GetStatus(void){return HAL_OK;}
void OLED_ShowString(uint8_t l,uint8_t c,const char *s){assert(l>=1 && l<=4);for(unsigned i=0;i+c<=16 && s[i];i++)rows[l-1][c+i-1]=s[i];}
void OLED_ShowSignedNum(uint8_t l,uint8_t c,int32_t n,uint8_t len){(void)l;(void)c;(void)n;(void)len;}
MotionState_t MotionState_Get(void){return MOTION_IDLE;}LightState_t LightState_Get(void){return LIGHT_NORMAL;}
LoggerStatus_t DataLogger_GetStatus(void){return LOGGER_READY;}uint32_t DataLogger_GetDropped(void){return 0;}
int main(void){
    UART_HandleTypeDef u;Debug_Init(&u);g_sensorData.mpuValid=g_sensorData.lightValid=1;g_sensorData.mpuTick=g_sensorData.lightTick=tick;
    const int16_t inputs[]={131,-131,0,INT16_MAX,INT16_MIN};
    const char *expected[]={"GX(dps)=1.00","GX(dps)=-1.00","GX(dps)=0.00","GX(dps)=250.12","GX(dps)=-250.13"};
    for(unsigned i=0;i<5;i++){out[0]=0;g_sensorData.mpu.gx=inputs[i];g_sensorData.gxFiltered=inputs[i];Monitor_Update();assert(strstr(out,expected[i]));}
    g_sensorData.mpuValid=g_sensorData.lightValid=0;out[0]=0;Monitor_Update();
    assert(strstr(out,"MPU INVALID") && strstr(out,"LIGHT INVALID"));assert(!strstr(out,"GX(dps)="));
    assert(!strncmp(rows[1],"MPU INVALID",11));assert(!strncmp(rows[2],"NO VALID DATA",13));assert(!strncmp(rows[3],"L:ERROR M:ERROR ",16));
    g_sensorData.mpuValid=g_sensorData.lightValid=1;out[0]=0;Monitor_Update();assert(strstr(out,"GX(dps)="));
    tick+=251;out[0]=0;Monitor_Update();assert(strstr(out,"MPU INVALID") && strstr(out,"LIGHT INVALID"));
    return 0;
}
