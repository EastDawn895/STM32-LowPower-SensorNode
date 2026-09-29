#include "light_sensor.h"

HAL_StatusTypeDef LightSensor_Init(ADC_HandleTypeDef *hadc)
{
    /* F1 的 ADC 上电校准：只做一次，明显改善精度 */
    if (hadc == NULL) {
        return HAL_ERROR;
    }

    return HAL_ADCEx_Calibration_Start(hadc);
}

HAL_StatusTypeDef LightSensor_ReadRaw(ADC_HandleTypeDef *hadc, uint16_t *raw)
{
    HAL_StatusTypeDef status;

    if ((hadc == NULL) || (raw == NULL)) {
        return HAL_ERROR;
    }

    /* 软件触发、单次转换三步走 */
    status = HAL_ADC_Start(hadc);
    if (status != HAL_OK) {
        return status;
    }

    status = HAL_ADC_PollForConversion(hadc, 10);   /* 等完成，超时 10ms */
    if (status != HAL_OK) {
        HAL_ADC_Stop(hadc);
        return status;
    }

    *raw = (uint16_t)HAL_ADC_GetValue(hadc);        /* 12 位结果 0~4095 */
    return HAL_ADC_Stop(hadc);
}
