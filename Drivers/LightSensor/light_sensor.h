#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "stm32f1xx_hal.h"

HAL_StatusTypeDef LightSensor_Init(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef LightSensor_ReadRaw(ADC_HandleTypeDef *hadc, uint16_t *raw);

#endif
