#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H
#include "sensor_state.h"


void SensorManager_Init(void);

void SensorManager_Update(void);

SensorData_t* SensorManager_GetData(void);

#endif