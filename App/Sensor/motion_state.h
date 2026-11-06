#ifndef MOTION_STATE_H
#define MOTION_STATE_H

#define MOTION_ACTIVE_THRES 2000  // 绝对值大于2000（约15°/s），认为是运动
#define MOTION_IDLE_THRES   1000  // 绝对值小于1000，认为是静止
#define MOTION_CONFIRM_MS   300   // 确认时间 300ms (运动响应可以比光照稍微快一点)

#include <stdint.h>

typedef enum
{
    MOTION_IDLE,
    MOTION_ACTIVE
} MotionState_t;


void MotionState_Init(void);
MotionState_t MotionState_Get(void);
void MotionState_Invalidate(void);
void MotionState_Update(int32_t gxFiltered);



#endif