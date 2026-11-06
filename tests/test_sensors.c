#include <assert.h>
#include <limits.h>
#include <string.h>
#include "sensor_manager.h"
#include "light_state.h"
#include "motion_state.h"
#include "light_sensor.h"
I2C_HandleTypeDef hi2c2; ADC_HandleTypeDef hadc1;
static uint32_t tick;static int mpuFail,adcFail,wrongID;static int16_t gx;
uint32_t HAL_GetTick(void){return tick;}
void HAL_Delay(uint32_t t){tick+=t;}
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h,uint16_t a,uint16_t reg,uint16_t sz,uint8_t *b,uint16_t n,uint32_t timeout){
    (void)h;assert(a==(0x68u<<1));assert(sz==1);assert(timeout==10);if(mpuFail)return HAL_TIMEOUT;
    memset(b,0,n);if(reg==0x75){assert(n==1);b[0]=wrongID?0:0x68;}
    else {assert(reg==0x3b && n==14);b[8]=(uint8_t)((uint16_t)gx>>8);b[9]=(uint8_t)gx;}
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h,uint16_t a,uint16_t r,uint16_t sz,uint8_t *b,uint16_t n,uint32_t t){
    (void)h;(void)a;(void)r;(void)sz;(void)b;(void)n;assert(t==10);return mpuFail?HAL_ERROR:HAL_OK;
}
HAL_StatusTypeDef LightSensor_Init(ADC_HandleTypeDef *h){(void)h;return adcFail?HAL_ERROR:HAL_OK;}
HAL_StatusTypeDef LightSensor_ReadRaw(ADC_HandleTypeDef *h,uint16_t *r){(void)h;if(adcFail)return HAL_ERROR;*r=3500;return HAL_OK;}
static void advance(unsigned ms){for(unsigned i=0;i<ms;i++){tick++;SensorManager_Service();if(tick%100u==0)SensorManager_Update();}}
int main(void){
    SensorManager_Init();assert(!g_sensorData.mpuValid);advance(2400);assert(g_sensorData.mpuValid);
    gx=31000;advance(1500);assert(g_sensorData.gxFiltered==31000);assert(MotionState_Get()==MOTION_ACTIVE);
    gx=-31000;advance(1500);assert(g_sensorData.gxFiltered==-31000);
    mpuFail=1;adcFail=1;advance(2000);assert(!g_sensorData.mpuValid && !g_sensorData.lightValid);
    mpuFail=0;adcFail=0;gx=0;advance(3500);assert(g_sensorData.mpuValid && g_sensorData.lightValid);
    /* Pending light confirmation does not survive failed reads or ADC recalibration. */
    LightState_Init();SensorManager_Update();assert(LightState_Get()==LIGHT_NORMAL);
    adcFail=1;advance(10000);adcFail=0;
    unsigned waited=0;while(!g_sensorData.lightValid && waited++<1100u)advance(1);
    assert(g_sensorData.lightValid);assert(LightState_Get()==LIGHT_NORMAL);
    advance(400);assert(LightState_Get()==LIGHT_NORMAL);advance(200);assert(LightState_Get()==LIGHT_LOW);
    SensorManager_Init();SensorManager_Update();
    for(unsigned repeat=0;repeat<3;repeat++) {
        advance(200);assert(LightState_Get()==LIGHT_NORMAL);
        adcFail=1;SensorManager_Update();assert(!g_sensorData.lightValid);
        adcFail=0;unsigned recovery=0;
        while(!g_sensorData.lightValid && recovery++<1100u)advance(1);
        assert(g_sensorData.lightValid && LightState_Get()==LIGHT_NORMAL);
    }
    advance(600);assert(LightState_Get()==LIGHT_LOW);
    assert(MPU6050_Init(0)==HAL_ERROR);assert(MPU6050_ReadData(&hi2c2,0)==HAL_ERROR);
    assert(MPU6050_CalibrateStart(0)==HAL_ERROR);wrongID=1;assert(MPU6050_Init(&hi2c2)==HAL_ERROR);wrongID=0;
    gx=-100;assert(MPU6050_CalibrateGyro(&hi2c2,2)==HAL_OK);
    MPU6050_Data_t d;gx=INT16_MAX;assert(MPU6050_ReadData(&hi2c2,&d)==HAL_OK);assert(d.gx==INT16_MAX);
    gx=100;assert(MPU6050_CalibrateGyro(&hi2c2,2)==HAL_OK);gx=INT16_MIN;assert(MPU6050_ReadData(&hi2c2,&d)==HAL_OK);assert(d.gx==INT16_MIN);
    assert(MPU6050_CalibrateStart(2)==HAL_OK);gx=0;assert(MPU6050_CalibrateStep(&hi2c2)==HAL_BUSY);gx=2000;assert(MPU6050_CalibrateStep(&hi2c2)==HAL_ERROR);
    return 0;
}
