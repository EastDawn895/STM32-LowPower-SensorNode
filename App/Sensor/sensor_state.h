#ifndef SENSOR_STATE_H
#define SENSOR_STATE_H

#include "mpu6050.h"
#include "stm32f1xx_hal.h"

typedef struct {
    MPU6050_Data_t mpu;
    int32_t gxFiltered;

    uint16_t lightRaw;
    int32_t lightFiltered;

    HAL_StatusTypeDef mpuRc;
    HAL_StatusTypeDef lightRc;
} SensorData_t;

extern SensorData_t g_sensorData;

#endif /* SENSOR_STATE_H */
