#include "motion_state.h"

#include "main.h"

static MotionState_t MotionState;
static MotionState_t PendingState;
static uint32_t confirmStartTick;


void MotionState_Init(void)
{
    MotionState=MOTION_IDLE;
    PendingState=MOTION_IDLE;
    confirmStartTick=0;
}

void MotionState_Invalidate(void) { PendingState=MotionState; confirmStartTick=0; }

MotionState_t MotionState_Get(void)
{
    return MotionState;
}

void MotionState_Update(int32_t gxFiltered)
{



    MotionState_t nextState = MotionState;
    uint32_t now = HAL_GetTick();
    switch (MotionState)
    {
    case MOTION_IDLE:
        {
            if (gxFiltered > MOTION_ACTIVE_THRES || gxFiltered < -MOTION_ACTIVE_THRES)nextState=MOTION_ACTIVE;
            break;
        }
    case MOTION_ACTIVE:
        {
            if (gxFiltered < MOTION_IDLE_THRES && gxFiltered > -MOTION_IDLE_THRES)nextState=MOTION_IDLE;
            break;
        }

    }
    if (nextState == MotionState)
    {
        PendingState=MotionState;
    }else
    {
        if (nextState != PendingState)
        {
            PendingState=nextState;
            confirmStartTick=now;
        }else
        {
            if (now-confirmStartTick>=MOTION_CONFIRM_MS)
            {
                MotionState=nextState;
            }
        }

    }

}
