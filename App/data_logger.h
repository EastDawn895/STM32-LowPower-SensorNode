#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <stdint.h>
#include "motion_state.h"
#include "light_state.h"

// 严格 32 字节对齐的日志结构体
typedef struct
{
    uint32_t Log_Time;     // 4 字节
    int32_t  Log_Value;    // 4 字节
    uint8_t  Log_Source;   // 1 字节: 比如 1=运动, 2=光照
    uint8_t  Log_LState;   // 1 字节: 触发时的状态枚举
    uint8_t  Log_Arr[22];  // 22 字节: 预留凑齐 32 字节
} SensorLog_t;

void DataLogger_Init(void);
void DataLogger_Update(void); // 放在主循环里不断扫描边沿

#endif