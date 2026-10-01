#include "sensor_manager.h"

#include "sensor_state.h"

#include "mpu6050.h"
#include "light_sensor.h"
#include "moving_avg.h"

#include "main.h"


extern I2C_HandleTypeDef hi2c2;
extern ADC_HandleTypeDef hadc1;


static MovingAvg_t f_gx;
static MovingAvg_t f_light;


void SensorManager_Init(void)
{
    g_sensorData.mpuRc =
        MPU6050_Init(&hi2c2);


    g_sensorData.lightRc =
        LightSensor_Init(&hadc1);


    MovingAvg_Init(&f_gx);

    MovingAvg_Init(&f_light);


    MPU6050_CalibrateGyro(&hi2c2,200);
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
        g_sensorData.gxFiltered =
            MovingAvg_Update(
                &f_gx,
                g_sensorData.mpu.gx
            );
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
    }

}