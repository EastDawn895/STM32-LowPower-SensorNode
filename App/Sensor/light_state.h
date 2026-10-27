#ifndef LIGHT_STATE_H
#define LIGHT_STATE_H

#define  LIGHT_LOW_IN 3200
#define  LIGHT_LOW_OUT 2800
#define  LIGHT_HIGH_IN 2000
#define  LIGHT_HIGH_OUT 1800
#define  LIGHT_STATE_CONFIRM_MS 500

#include <stdint.h>

typedef enum
{
    LIGHT_LOW,
    LIGHT_NORMAL,
    LIGHT_HIGH
} LightState_t;


void LightState_Init(void);
LightState_t LightState_Get(void);
void LightState_Update(int32_t lightFiltered);



#endif