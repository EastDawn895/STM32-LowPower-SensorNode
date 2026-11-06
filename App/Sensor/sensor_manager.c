#include "sensor_manager.h"
#include "light_sensor.h"
#include "light_state.h"
#include "motion_state.h"
#include "moving_avg.h"
extern I2C_HandleTypeDef hi2c2;
extern ADC_HandleTypeDef hadc1;
static MovingAvg_t gyro,light;
static uint32_t retryTick,phaseTick,lightRetry;
static uint8_t adcReady;
static enum { RETRY, SETTLE, CALIBRATE, RUNNING } phase;
static void invalidateMPU(void)
{g_sensorData.mpuValid=0;MovingAvg_Init(&gyro);MotionState_Invalidate();}
static void invalidateLight(void)
{g_sensorData.lightValid=0;MovingAvg_Init(&light);LightState_Invalidate();}
void SensorManager_Init(void)
{
    LightState_Init();MotionState_Init();invalidateMPU();invalidateLight();
    phase=RETRY;retryTick=HAL_GetTick()-1000u;
    g_sensorData.mpuRc=HAL_BUSY;g_sensorData.gyroCalRc=HAL_BUSY;
    g_sensorData.lightRc=LightSensor_Init(&hadc1);
    adcReady=(uint8_t)(g_sensorData.lightRc==HAL_OK);lightRetry=HAL_GetTick();
}
void SensorManager_Service(void)
{
    uint32_t now=HAL_GetTick();HAL_StatusTypeDef rc;
    switch(phase) {
    case RETRY:
        if(now-retryTick<1000u) return;
        retryTick=now;g_sensorData.mpuRc=MPU6050_Init(&hi2c2);
        if(g_sensorData.mpuRc==HAL_OK) {phase=SETTLE;phaseTick=HAL_GetTick();g_sensorData.gyroCalRc=HAL_BUSY;}
        break;
    case SETTLE:
        if(now-phaseTick>=100u) {
            (void)MPU6050_CalibrateStart(200);phase=CALIBRATE;phaseTick=now-10u;
        }
        break;
    case CALIBRATE:
        if(now-phaseTick<10u) return;
        phaseTick=now;rc=MPU6050_CalibrateStep(&hi2c2);g_sensorData.gyroCalRc=rc;
        if(rc==HAL_OK) phase=RUNNING;
        else if(rc!=HAL_BUSY) {phase=RETRY;retryTick=now;g_sensorData.mpuRc=rc;invalidateMPU();}
        break;
    case RUNNING: break;
    }
}
void SensorManager_Update(void)
{
    if(phase==RUNNING) {
        g_sensorData.mpuRc=MPU6050_ReadData(&hi2c2,&g_sensorData.mpu);
        if(g_sensorData.mpuRc==HAL_OK) {
            g_sensorData.gxFiltered=MovingAvg_Update(&gyro,g_sensorData.mpu.gx);
            g_sensorData.mpuValid=1;g_sensorData.mpuTick=HAL_GetTick();
            MotionState_Update(g_sensorData.gxFiltered);
        } else {invalidateMPU();phase=RETRY;retryTick=HAL_GetTick();}
    } else invalidateMPU();
    if(!adcReady && HAL_GetTick()-lightRetry>=1000u) {
        lightRetry=HAL_GetTick();g_sensorData.lightRc=LightSensor_Init(&hadc1);
        adcReady=(uint8_t)(g_sensorData.lightRc==HAL_OK);
    }
    if(adcReady) {
        g_sensorData.lightRc=LightSensor_ReadRaw(&hadc1,&g_sensorData.lightRaw);
        if(g_sensorData.lightRc==HAL_OK) {
            g_sensorData.lightFiltered=MovingAvg_Update(&light,g_sensorData.lightRaw);
            g_sensorData.lightValid=1;g_sensorData.lightTick=HAL_GetTick();
            LightState_Update(g_sensorData.lightFiltered);
        } else {invalidateLight();adcReady=0;lightRetry=HAL_GetTick();}
    } else invalidateLight();
}
SensorData_t *SensorManager_GetData(void) {return &g_sensorData;}
