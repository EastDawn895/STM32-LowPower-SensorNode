#include <assert.h>
#include <limits.h>
#include "oled.h"
TestDWT testDWT;TestDebug testDebug;uint32_t SystemCoreClock=72000000;
static uint32_t tick;static int scl=1,sda=1,started,bits,nack,stuck;static unsigned acknowledgements;
uint32_t HAL_GetTick(void){testDWT.CYCCNT+=72u;return tick+testDWT.CYCCNT/72000u;}
void HAL_Delay(uint32_t n){tick+=n;}
void HAL_GPIO_Init(int p,GPIO_InitTypeDef *g){(void)p;assert(g->Mode==GPIO_MODE_OUTPUT_OD);}
void HAL_GPIO_WritePin(int p,int pin,int v){
    (void)p;
    if(pin==GPIO_PIN_8){if(!scl && v && started)bits++;scl=v;}
    else {if(scl && sda && !v){started=1;bits=0;}if(scl && !sda && v)started=0;sda=v;}
}
int HAL_GPIO_ReadPin(int p,int pin){
    (void)p;if(pin==GPIO_PIN_8)return stuck?0:scl;
    if(started && bits && bits%9==0){assert(sda==1);acknowledgements++;return nack?1:0;}
    return sda;
}
int main(void){
    OLED_Init();assert(OLED_GetStatus()==HAL_OK);assert(acknowledgements==27);
    for(int i=0;i<64;i++)OLED_Service();
    assert(OLED_GetStatus()==HAL_OK);
    OLED_ShowString(1,1,"SensorNode");OLED_ShowSignedNum(2,1,INT32_MIN,10);
    OLED_ShowChar(0,0,(char)255);OLED_ShowChar(255,255,'A');OLED_ShowString(1,16,"ABCDE");OLED_ShowString(1,1,0);
    for(int i=0;i<64;i++)OLED_Service();
    nack=1;OLED_Clear();OLED_Service();assert(OLED_GetStatus()==HAL_ERROR);
    nack=0;tick+=1000;OLED_Service();assert(OLED_GetStatus()==HAL_OK);
    stuck=1;OLED_Service();assert(OLED_GetStatus()==HAL_TIMEOUT);assert(scl==1 && sda==1);
    stuck=0;tick+=1000;OLED_Service();assert(OLED_GetStatus()==HAL_OK);
    return 0;
}
