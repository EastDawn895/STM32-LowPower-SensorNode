#include <assert.h>
#include <string.h>
#include "w25q64.h"
SPI_HandleTypeDef hspi2;
static uint32_t tick;static int selected,mode;static uint8_t op,wel;static HAL_StatusTypeDef failure;static int payloadError,readError,noWel;
uint32_t HAL_GetTick(void){return tick++;}
void HAL_Delay(uint32_t t){tick+=t;}
void HAL_GPIO_WritePin(int p,int n,int v){(void)p;(void)n;selected=!v;if(!selected)op=0;}
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *h,uint8_t *b,uint16_t n,uint32_t t){
    (void)h;assert(selected);assert(t<=10u);if(failure)return failure;
    if(op==2 && payloadError)return HAL_TIMEOUT;
    if(!op){op=b[0];assert(op==5||op==6||op==0x9f||op==3||op==2||op==0x20);if(op==6)wel=noWel?0u:2u;}
    (void)n;return HAL_OK;
}
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *h,uint8_t *b,uint16_t n,uint32_t t){
    (void)h;(void)t;assert(selected);if(failure)return failure;
    if(op==3 && readError)return HAL_ERROR;
    if(op==0x9f){assert(n==3);b[0]=mode==2?0xFF:0xEF;b[1]=0x40;b[2]=0x17;}
    else if(op==5){assert(n==1);b[0]=mode==1?1u:(mode==3?255u:wel);}
    else memset(b,0,n);
    return HAL_OK;
}
int main(void){
    uint8_t b[32]={0}; assert(W25Q64_Init()==HAL_OK);assert(!selected);
    assert(W25Q64_PageProgram(0,b,32)==HAL_OK);assert(!selected);
    assert(W25Q64_PageProgram(250,b,32)==HAL_ERROR);assert(W25Q64_ReadData(W25Q64_CAPACITY,b,1)==HAL_ERROR);
    assert(W25Q64_EraseSector(1)==HAL_ERROR);assert(W25Q64_ReadData(0,0,1)==HAL_ERROR);
    readError=1;assert(W25Q64_ReadData(0,b,1)==HAL_ERROR);assert(!selected);readError=0;
    payloadError=1;assert(W25Q64_PageProgram(0,b,32)==HAL_TIMEOUT);assert(!selected);payloadError=0;
    noWel=1;assert(W25Q64_BeginErase(0)==HAL_ERROR);noWel=0;
    mode=1;tick=0xFFFFFF00u;assert(W25Q64_Init()==HAL_TIMEOUT);assert(!selected);
    mode=2;assert(W25Q64_Init()==HAL_ERROR);mode=3;assert(W25Q64_Init()==HAL_ERROR);
    mode=0;failure=HAL_ERROR;assert(W25Q64_Init()==HAL_ERROR);assert(!selected);
    failure=HAL_TIMEOUT;assert(W25Q64_Init()==HAL_TIMEOUT);failure=HAL_OK;assert(W25Q64_Init()==HAL_OK);
    assert(W25Q64_BeginErase(0)==HAL_OK);mode=1;tick+=501;assert(W25Q64_Poll()==HAL_TIMEOUT);
    return 0;
}
