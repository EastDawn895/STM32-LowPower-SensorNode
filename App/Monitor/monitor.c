#include "monitor.h"
#include "sensor_state.h"
#include "light_state.h"
#include "motion_state.h"
#include "oled.h"
#include "debug.h"
#include "data_logger.h"

void Monitor_Init(void)
{
    OLED_Init();
    OLED_ShowString(1, 1, "SensorNode");
}

void Monitor_Update(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t m = (uint8_t)(g_sensorData.mpuValid && now - g_sensorData.mpuTick <= 250u);
    uint8_t l = (uint8_t)(g_sensorData.lightValid && now - g_sensorData.lightTick <= 250u);
    Debug_BeginFrame();
    if (m)
    {
        Debug_Print("AX(g)=");
        Debug_PrintFixed((int32_t)g_sensorData.mpu.ax * 100 / 16384, 2);
        Debug_Print(" GX(dps)=");
        Debug_PrintFixed((int32_t)g_sensorData.mpu.gx * 100 / 131, 2);
        Debug_Print(" GXF(dps)=");
        Debug_PrintFixed(g_sensorData.gxFiltered * 100 / 131, 2);
        OLED_ShowString(2, 1, "AX(raw)=        ");
        OLED_ShowSignedNum(2, 9, g_sensorData.mpu.ax, 6);
        OLED_ShowString(3, 1, "GX(raw)=        ");
        OLED_ShowSignedNum(3, 9, g_sensorData.mpu.gx, 6);
        const char* s = MotionState_Get() == MOTION_ACTIVE ? "ACTIVE" : "IDLE  ";
        OLED_ShowString(4, 9, "M:");
        OLED_ShowString(4, 11, s);
        Debug_Print(" M=");
        Debug_Print(s);
    }
    else
    {
        Debug_Print("MPU INVALID rc=");
        Debug_PrintInt(g_sensorData.mpuRc);
        Debug_Print(" cal=");
        Debug_PrintInt(g_sensorData.gyroCalRc);
        OLED_ShowString(2, 1, "MPU INVALID     ");
        OLED_ShowString(3, 1, "NO VALID DATA   ");
        OLED_ShowString(4, 9, "M:ERROR ");
    }
    if (l)
    {
        Debug_Print(" Light(raw)=");
        Debug_PrintUInt(g_sensorData.lightRaw);
        Debug_Print(" F=");
        Debug_PrintInt(g_sensorData.lightFiltered);
        LightState_t state = LightState_Get();
        const char* s = state == LIGHT_LOW ? "LOW   " : state == LIGHT_HIGH ? "HIGH  " : "NORMAL";
        Debug_Print(" L=");
        Debug_Print(s);
        OLED_ShowString(4, 1, "L:");
        OLED_ShowString(4, 3, s);
    }
    else
    {
        Debug_Print(" LIGHT INVALID rc=");
        Debug_PrintInt(g_sensorData.lightRc);
        OLED_ShowString(4, 1, "L:ERROR ");
    }
    Debug_Print(" Log=");
    Debug_PrintInt(DataLogger_GetStatus());
    Debug_Print(" Drop=");
    Debug_PrintUInt(DataLogger_GetDropped());
    Debug_Print(" OLED=");
    Debug_PrintInt(OLED_GetStatus());
    Debug_Print("\r\n");
    Debug_EndFrame();
}
