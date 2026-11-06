#include <assert.h>
#include <limits.h>
#include "light_state.h"
#include "motion_state.h"
static uint32_t tick;
uint32_t HAL_GetTick(void) {return tick;}
static void hold(int32_t v) {for(int i=0;i<20;i++){LightState_Update(v);tick+=100;}}
int main(void) {
    LightState_Init();hold(1900);assert(LightState_Get()==LIGHT_NORMAL);
    hold(1800);assert(LightState_Get()==LIGHT_NORMAL);hold(1799);assert(LightState_Get()==LIGHT_HIGH);
    hold(2000);assert(LightState_Get()==LIGHT_HIGH);hold(2001);assert(LightState_Get()==LIGHT_NORMAL);
    hold(3200);assert(LightState_Get()==LIGHT_NORMAL);hold(3201);assert(LightState_Get()==LIGHT_LOW);
    hold(2800);assert(LightState_Get()==LIGHT_LOW);hold(2799);assert(LightState_Get()==LIGHT_NORMAL);
    for(int i=0;i<100;i++){LightState_Update(i%2?1799:1801);tick+=100;}assert(LightState_Get()==LIGHT_NORMAL);
    LightState_Update(3500);tick+=10000;LightState_Invalidate();LightState_Update(3500);assert(LightState_Get()==LIGHT_NORMAL);
    tick+=499;LightState_Update(3500);assert(LightState_Get()==LIGHT_NORMAL);tick++;LightState_Update(3500);assert(LightState_Get()==LIGHT_LOW);
    LightState_Init();tick=UINT32_MAX-250u;LightState_Update(3500);tick+=500;LightState_Update(3500);assert(LightState_Get()==LIGHT_LOW);
    LightState_Init();hold(3500);assert(LightState_Get()==LIGHT_LOW);
    LightState_Update(0);tick+=500;LightState_Update(0);assert(LightState_Get()==LIGHT_NORMAL);
    LightState_Update(0);tick+=500;LightState_Update(0);assert(LightState_Get()==LIGHT_HIGH);
    MotionState_Init();MotionState_Update(2000);tick+=300;MotionState_Update(2000);assert(MotionState_Get()==MOTION_IDLE);
    MotionState_Update(-2001);tick+=300;MotionState_Update(-2001);assert(MotionState_Get()==MOTION_ACTIVE);
    MotionState_Update(-1000);tick+=300;MotionState_Update(-1000);assert(MotionState_Get()==MOTION_ACTIVE);
    MotionState_Init();MotionState_Update(INT32_MIN);tick+=300;MotionState_Update(INT32_MIN);assert(MotionState_Get()==MOTION_ACTIVE);
    MotionState_Update(0);MotionState_Invalidate();tick+=10000;MotionState_Update(0);assert(MotionState_Get()==MOTION_ACTIVE);
    tick+=300;MotionState_Update(0);assert(MotionState_Get()==MOTION_IDLE);
    return 0;
}
