#include "app.h"
#include "main.h"
#include "debug.h"
#include "sensor_manager.h"
#include "monitor.h"
#include "sensor_state.h"

static uint32_t s_sensorTick = 0;    /* 100ms 节拍自己的时基 */
static uint32_t s_lastTick = 0;

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

    /* 100ms 节拍：读传感器 → 灌滤波器。只更新数据，不打印 */
    if (now - s_sensorTick >= 100u) {
        s_sensorTick = now;
        SensorManager_Update();

    }

    if (now - s_lastTick >= 1000u)
    {
        s_lastTick = now;

        HAL_GPIO_TogglePin(
        LED_GPIO_Port,
        LED_Pin
    );
        Monitor_Update();

    }
}
