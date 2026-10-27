#include "monitor.h"
#include "sensor_state.h"
#include "light_state.h"
#include "oled.h"
#include "debug.h"
#include "motion_state.h"


static void PrintSensorData(void);


void Monitor_Init(void)
{
    OLED_Init();

    OLED_ShowString(1, 1, "SensorNode");
}


void Monitor_Update(void)
{
    PrintSensorData();
}


static void PrintSensorData(void)
{
    if (g_sensorData.mpuRc == HAL_OK)
    {
        int32_t ax =
            (int32_t)g_sensorData.mpu.ax
            * 100 / 16384;


        int32_t gx =
            (int32_t)g_sensorData.mpu.gx
            * 100 / 131;


        int32_t gx_f =
            g_sensorData.gxFiltered
            * 100 / 131;


        Debug_Print("AX=");
        Debug_PrintFixed(ax, 2);


        Debug_Print(" Gx=");
        Debug_PrintFixed(gx, 1);


        Debug_Print(" GxF=");
        Debug_PrintFixed(gx_f, 1);


        OLED_ShowString(
            2,
            1,
            "AX="
        );


        OLED_ShowSignedNum(
            2,
            4,
            g_sensorData.mpu.ax,
            6
        );


        OLED_ShowString(
            3,
            1,
            "GX="
        );


        OLED_ShowSignedNum(
            3,
            4,
            g_sensorData.mpu.gx,
            6
        );


        MotionState_t Motion_state = MotionState_Get();
        const char* stateStrM = "UNKNOWN";

        switch (Motion_state)
        {
        case MOTION_IDLE: stateStrM = "IDLE  ";
            break;
        case MOTION_ACTIVE: stateStrM = "ACTIVE";
            break;
        }

        Debug_Print(" State_M=");
        Debug_Print((char*)stateStrM);

        OLED_ShowString(4, 9, "M:");
        OLED_ShowString(4, 11, (char*)stateStrM);
    }
    else
    {
        Debug_Print("MPU ERROR=");
        Debug_PrintInt(
            g_sensorData.mpuRc
        );
    }


    if (g_sensorData.lightRc == HAL_OK)
    {
        Debug_Print(" Light=");

        Debug_PrintInt(
            g_sensorData.lightRaw
        );


        Debug_Print(" Filter=");

        Debug_PrintInt(
            g_sensorData.lightFiltered
        );


        LightState_t Light_state = LightState_Get();
        const char* stateStrL = "UNKNOWN";

        switch (Light_state)
        {
        case LIGHT_LOW: stateStrL = "LOW   ";
            break;
        case LIGHT_NORMAL: stateStrL = "NORMAL";
            break;
        case LIGHT_HIGH: stateStrL = "HIGH  ";
            break;
        }

        Debug_Print(" State_L=");
        Debug_Print((char*)stateStrL);

        OLED_ShowString(4, 1, "L:");
        OLED_ShowString(4, 3, (char*)stateStrL);
    }


    Debug_Print("\r\n");
}
