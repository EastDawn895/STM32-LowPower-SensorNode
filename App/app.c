#include "app.h"
#include "main.h"

#include "sensor_state.h"
#include "sensor_manager.h"
#include "debug.h"
#include "oled.h"
#include "monitor.h"

static HAL_StatusTypeDef s_mpuInitRc = HAL_ERROR;
static HAL_StatusTypeDef s_gyroCalRc = HAL_ERROR;
static uint32_t s_mpuReadCount;
static uint32_t s_mpuFailCount;
static HAL_StatusTypeDef s_mpuLastFailRc;
static uint32_t s_sensorTick = 0;    /* 100ms 节拍自己的时基 */
static uint32_t s_lastTick = 0;
extern I2C_HandleTypeDef hi2c2;
extern ADC_HandleTypeDef hadc1;


static void Sensor_Task_100ms(void);
static void Monitor_Task_1000ms(void);

void App_Init(void)
{
    Debug_Init(&huart1);
    SensorManager_Init();
    Debug_Print("MPU Init rc=");
    Debug_PrintInt((int32_t)s_mpuInitRc);
    Debug_Print("\r\n");

    Debug_Print("Gyro Cal rc=");
    Debug_PrintInt((int32_t)s_gyroCalRc);
    Debug_Print("\r\n");

    Debug_Print("Light Init rc=");
    Debug_PrintInt((int32_t)g_sensorData.lightRc);
    Debug_Print("\r\n");
    Debug_Print("System Init OK\r\n");
    uint8_t who = MPU6050_ReadWhoAmI(&hi2c2);
    Debug_Print("WHO_AM_I = ");
    Debug_PrintHex(who);
    Debug_Print("\r\n");
    OLED_Init();
    OLED_ShowString(1, 1, "SensorNode");

}

void App_Run(void)
{
    uint32_t now = HAL_GetTick();

    /* 100ms 节拍：读传感器 → 灌滤波器。只更新数据，不打印 */
    if (now - s_sensorTick >= 100u) {
        s_sensorTick = now;
        Sensor_Task_100ms();

    }

    if (now - s_lastTick >= 1000u)
    {
        s_lastTick = now;
        Monitor_Task_1000ms();

    }
}



static void Sensor_Task_100ms(void) {
    s_mpuReadCount++;

    SensorManager_Update();

}
static void Monitor_Task_1000ms(void) {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

    if (g_sensorData.mpuRc == HAL_OK)
    {
        /* 物理量换算：数据全部来自 100ms 层的邮箱 */
        int32_t ax = (int32_t)g_sensorData.mpu.ax * 100 / 16384;
        int32_t ay = (int32_t)g_sensorData.mpu.ay * 100 / 16384;
        int32_t az = (int32_t)g_sensorData.mpu.az * 100 / 16384;
        int32_t gx = (int32_t)g_sensorData.mpu.gx * 100 / 131;
        int32_t gy = (int32_t)g_sensorData.mpu.gy * 100 / 131;
        int32_t gz = (int32_t)g_sensorData.mpu.gz * 100 / 131;
        int32_t gx_f = (int32_t)g_sensorData.gxFiltered * 100 / 131;
        Debug_Print("AX="); Debug_PrintFixed(ax, 2);
        Debug_Print(" AY="); Debug_PrintFixed(ay, 2);
        Debug_Print(" AZ="); Debug_PrintFixed(az, 2);
        Debug_Print(" | Gx="); Debug_PrintFixed(gx, 1);
        Debug_Print(" Gx_F="); Debug_PrintFixed(gx_f, 1);
        Debug_Print(" Gy="); Debug_PrintFixed(gy, 1);
        Debug_Print(" Gz="); Debug_PrintFixed(gz, 1);
        OLED_ShowString(2, 1, "AX=");
        OLED_ShowSignedNum(2, 4, g_sensorData.mpu.ax, 6);
        OLED_ShowString(3, 1, "Gx=");
        OLED_ShowSignedNum(3, 4, g_sensorData.mpu.gx, 6);
    }
    else
    {
        Debug_Print("FAIL rc="); Debug_PrintInt((int32_t)g_sensorData.mpuRc);
    }

    if (g_sensorData.lightRc == HAL_OK)
    {
        Debug_Print(" LightRaw="); Debug_PrintInt((int32_t)g_sensorData.lightRaw);
        Debug_Print(" LightF="); Debug_PrintInt(g_sensorData.lightFiltered);
    }
    else
    {
        Debug_Print(" Light_FAIL rc="); Debug_PrintInt((int32_t)g_sensorData.lightRc);
    }
    Debug_Print(" MPU_Reads=");
    Debug_PrintInt((int32_t)s_mpuReadCount);
    Debug_Print(" MPU_Fails=");
    Debug_PrintInt((int32_t)s_mpuFailCount);
    Debug_Print(" LastRC=");
    Debug_PrintInt((int32_t)s_mpuLastFailRc);
    s_mpuReadCount = 0;
    s_mpuFailCount = 0;
    Debug_Print("\r\n");
}