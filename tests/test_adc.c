#include <assert.h>
#include "light_sensor.h"
static HAL_StatusTypeDef startRc,pollRc,stopRc;static unsigned stops;
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *a){(void)a;return startRc;}
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *a){(void)a;return startRc;}
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *a,uint32_t t){(void)a;assert(t==10);return pollRc;}
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *a){(void)a;stops++;return stopRc;}
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *a){(void)a;return 4095;}
int main(void){
    ADC_HandleTypeDef a;uint16_t raw=42;
    assert(LightSensor_Init(0)==HAL_ERROR);assert(LightSensor_ReadRaw(0,&raw)==HAL_ERROR);assert(LightSensor_ReadRaw(&a,0)==HAL_ERROR);
    assert(LightSensor_Init(&a)==HAL_OK);assert(LightSensor_ReadRaw(&a,&raw)==HAL_OK && raw==4095);
    raw=42;pollRc=HAL_TIMEOUT;assert(LightSensor_ReadRaw(&a,&raw)==HAL_TIMEOUT && raw==42 && stops==2);
    pollRc=HAL_OK;startRc=HAL_ERROR;assert(LightSensor_Init(&a)==HAL_ERROR);assert(LightSensor_ReadRaw(&a,&raw)==HAL_ERROR && stops==2);
    startRc=HAL_OK;stopRc=HAL_ERROR;assert(LightSensor_ReadRaw(&a,&raw)==HAL_ERROR);return 0;
}
