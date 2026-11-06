#include "app.h"
#include "main.h"
#include "debug.h"
#include "sensor_manager.h"
#include "monitor.h"
#include "light_state.h"
#include "motion_state.h"
#include "w25q64.h"
#include "data_logger.h"
#include "oled.h"
static uint32_t sensorTick, monitorTick, ledTick;

void App_Init(void)
{
    Debug_Init(&huart1);
    SensorManager_Init();
    HAL_StatusTypeDef flash = W25Q64_Init();
    /* Read-only recovery also resets the logger to offline if Flash init failed. */
    (void)DataLogger_Init();
    Debug_BeginFrame();
    Debug_Print("Startup Flash=");
    Debug_PrintInt(flash);
    Debug_Print(" Logger=");
    Debug_PrintInt(DataLogger_GetStatus());
    Debug_Print("; MPU calibration pending (keep still)\r\n");
    Debug_EndFrame();
    Monitor_Init();
    sensorTick = monitorTick = ledTick = HAL_GetTick();
}

void App_Run(void)
{
    SensorManager_Service();
    uint32_t now = HAL_GetTick();
    if (now - sensorTick >= 100u)
    {
        sensorTick = now;
        SensorManager_Update();
    }
    DataLogger_Update();
    OLED_Service();
    if (now - monitorTick >= 1000u)
    {
        monitorTick = now;
        Monitor_Update();
    }
    uint32_t interval = 1000u;
    if (!g_sensorData.mpuValid || !g_sensorData.lightValid || DataLogger_GetStatus() != LOGGER_READY || OLED_GetStatus()
        != HAL_OK || Debug_GetStatus() != HAL_OK)
        interval = 100u;
    else if (LightState_Get() != LIGHT_NORMAL || MotionState_Get() == MOTION_ACTIVE)interval = 250u;
    if (now - ledTick >= interval)
    {
        ledTick = now;
        HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
    }
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON,PWR_SLEEPENTRY_WFI);
}
