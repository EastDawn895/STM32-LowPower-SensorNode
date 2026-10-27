#include "light_state.h"
#include "main.h"

static LightState_t LightState;
static LightState_t PendingState;
static uint32_t confirmStartTick;

void LightState_Init(void)
{
    LightState=LIGHT_NORMAL;
    PendingState=LIGHT_NORMAL;
    confirmStartTick=0;
}

LightState_t LightState_Get(void)
{
    return LightState;
}

void LightState_Update(int32_t lightFiltered)
{
    LightState_t nextState = LightState;
    uint32_t now = HAL_GetTick();
    switch (LightState)
    {
        case LIGHT_NORMAL:
            {
                if (lightFiltered >LIGHT_LOW_IN)nextState=LIGHT_LOW;
                else if (lightFiltered <LIGHT_HIGH_IN)nextState=LIGHT_HIGH;
                break;
            }
        case LIGHT_LOW:
            {
                if (lightFiltered <LIGHT_LOW_OUT)nextState=LIGHT_NORMAL;
                break;
            }
        case LIGHT_HIGH:
            {
                if (lightFiltered >LIGHT_HIGH_OUT)nextState=LIGHT_NORMAL;
                break;
            }

    }
    if (nextState == LightState)
    {
        PendingState=LightState;
    }else
    {
        if (nextState != PendingState)
        {
            PendingState=nextState;
            confirmStartTick=now;
        }else
        {
            if (now-confirmStartTick>=LIGHT_STATE_CONFIRM_MS)
            {
                LightState=nextState;
            }
        }

    }

}

