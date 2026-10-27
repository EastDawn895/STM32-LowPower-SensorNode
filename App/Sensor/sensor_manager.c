#include "sensor_manager.h"

#include "sensor_state.h"

#include "mpu6050.h"
#include "light_sensor.h"
#include "light_state.h"
#include "moving_avg.h"
#include "debug.h"
#include "motion_state.h"

#include "main.h"


extern I2C_HandleTypeDef hi2c2;
extern ADC_HandleTypeDef hadc1;


static MovingAvg_t f_gx;
static MovingAvg_t f_light; 


void SensorManager_Init(void)
{
    MovingAvg_Init(&f_gx);
    MovingAvg_Init(&f_light);
    LightState_Init();
    MotionState_Init();

    g_sensorData.mpuRc =
        MPU6050_Init(&hi2c2);


    if(g_sensorData.mpuRc == HAL_OK)
    {
        uint8_t who = MPU6050_ReadWhoAmI(&hi2c2);

        Debug_Print("WHO_AM_I=");
        Debug_PrintHex(who);
        Debug_Print("\r\n");


        g_sensorData.gyroCalRc =
            MPU6050_CalibrateGyro(&hi2c2,200);
    }
    else
    {
        g_sensorData.gyroCalRc = HAL_ERROR;
    }


    g_sensorData.lightRc =
        LightSensor_Init(&hadc1);

}



void SensorManager_Update(void)
{

    g_sensorData.mpuRc =
        MPU6050_ReadData(
            &hi2c2,
            &g_sensorData.mpu
        );


    if(g_sensorData.mpuRc == HAL_OK)
    {

        int16_t gx = g_sensorData.mpu.gx;


        if(gx > 30000 || gx < -30000)
        {
            gx = 0;
        }
        
        g_sensorData.gxFiltered =
            MovingAvg_Update(
                &f_gx,
                gx
            );
        MotionState_Update(g_sensorData.gxFiltered);

    }



    g_sensorData.lightRc =
        LightSensor_ReadRaw(
            &hadc1,
            &g_sensorData.lightRaw
        );


    if(g_sensorData.lightRc == HAL_OK)
    {
        g_sensorData.lightFiltered =
            MovingAvg_Update(
                &f_light,
                g_sensorData.lightRaw
            );
        LightState_Update(g_sensorData.lightFiltered);
    }

}