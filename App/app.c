#include "app.h"
#include "main.h"
#include "debug.h"
#include "sensor_manager.h"
#include "monitor.h"
#include "sensor_state.h"
#include "light_state.h"
#include "motion_state.h"

static uint32_t s_sensorTick = 0;    /* 100ms 节拍自己的时基 */
static uint32_t s_monitorTick = 0;   /* 1000ms 打印显示时基 (把原来的 s_lastTick 改名) */
static uint32_t s_ledTick = 0;

void App_Init(void)
{
    Debug_Init(&huart1);

    SensorManager_Init();

    Debug_Print("MPU=");
    Debug_PrintInt(g_sensorData.mpuRc);

    Debug_Print(" Light=");
    Debug_PrintInt(g_sensorData.lightRc);

    Debug_Print(" GyroCal=");
    Debug_PrintInt(g_sensorData.gyroCalRc);

    Debug_Print("\r\n");


    Monitor_Init();


    Debug_Print("System Init OK\r\n");
}
void App_Run(void)
{
    uint32_t now = HAL_GetTick();

    uint32_t ledInterval = 1000; // 默认 1000ms 慢闪 (NORMAL)

    // 判断 1：最高优先级是硬件错误 (ERROR)
    if (g_sensorData.mpuRc != HAL_OK || g_sensorData.lightRc != HAL_OK)
    {
        ledInterval = 100; // 100ms 狂闪，提示硬件掉线
    }
    // 判断 2：硬件正常，但传感器读数异常 (ABNORMAL)
    
    else if (LightState_Get() != LIGHT_NORMAL || MotionState_Get() == MOTION_ACTIVE)
    {
        ledInterval = 250; // 250ms 快闪，提示环境光照异常或检测到运动
    }
    // 否则维持 1000ms 慢闪

    /* 100ms 节拍：读传感器 → 灌滤波器。只更新数据，不打印 */
    if (now - s_sensorTick >= 100u) {
        s_sensorTick = now;
        SensorManager_Update();

    }

    if (now - s_monitorTick >= 1000u)
    {
        s_monitorTick = now;

        Monitor_Update();

    }

    if (now - s_ledTick >= ledInterval)
    {
        s_ledTick = now;

        HAL_GPIO_TogglePin(
        LED_GPIO_Port,
        LED_Pin
    );

    }
}
