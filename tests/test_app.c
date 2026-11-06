#include <assert.h>
#include <string.h>
#include "app.h"
#include "sensor_manager.h"
#include "data_logger.h"
#include "motion_state.h"
#include "light_state.h"
#include "oled.h"
UART_HandleTypeDef huart1;SPI_HandleTypeDef hspi2;
static uint32_t tick;static unsigned samples,monitors,sleeps,services;
uint32_t HAL_GetTick(void){return tick++;}
void HAL_Delay(uint32_t n){tick+=n;}
void HAL_GPIO_WritePin(int p,int n,int v){(void)p;(void)n;(void)v;}
void HAL_GPIO_TogglePin(int p,int n){(void)p;(void)n;}
void HAL_PWR_EnterSLEEPMode(int r,int e){(void)r;(void)e;sleeps++;}
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *h,uint8_t *b,uint16_t n,uint32_t t){(void)h;(void)n;(void)t;assert(b[0]!=0x20 && b[0]!=0x02);return HAL_OK;}
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *h,uint8_t *b,uint16_t n,uint32_t t){(void)h;(void)t;memset(b,255,n);return HAL_OK;}
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *h,uint8_t *b,uint16_t n,uint32_t t){(void)h;(void)b;(void)n;(void)t;return HAL_OK;}
void SensorManager_Init(void){}
void SensorManager_Service(void){services++;}
void SensorManager_Update(void){samples++;}
void Monitor_Init(void){}void Monitor_Update(void){monitors++;}
void OLED_Service(void){}HAL_StatusTypeDef OLED_GetStatus(void){return HAL_ERROR;}
MotionState_t MotionState_Get(void){return MOTION_IDLE;}LightState_t LightState_Get(void){return LIGHT_NORMAL;}
int main(void){
    App_Init();assert(DataLogger_GetStatus()==LOGGER_IO_ERROR);
    for(int i=0;i<2000;i++)App_Run();
    assert(samples>10 && monitors>1 && sleeps==2000 && services==2000);return 0;
}
