#include "app.h"
#include "debug.h"
#include "main.h"
#include "mpu6050.h"
#include "oled.h"
#include "moving_avg.h"              /* 新增 include */
#include "light_sensor.h"

static HAL_StatusTypeDef s_mpuInitRc = HAL_ERROR;
static HAL_StatusTypeDef s_gyroCalRc = HAL_ERROR;
static uint32_t s_mpuReadCount;
static uint32_t s_mpuFailCount;
static HAL_StatusTypeDef s_mpuLastFailRc;
static uint32_t s_sensorTick = 0;    /* 100ms 节拍自己的时基 */
static MovingAvg_t f_gx;             /* Gx 的滤波器实例 */
static MovingAvg_t f_light;          /* 光敏传感器的滤波器实例 */
static MPU6050_Data_t s_sample;      /* 邮箱1：100ms 写，1Hz 读 */
static HAL_StatusTypeDef s_sampleRc = HAL_ERROR; /* MPU6050 最近一次读取成败 */
static HAL_StatusTypeDef s_lightRc = HAL_ERROR;  /* 光敏传感器最近一次读取成败 */
static uint16_t s_lightRaw;                      /* 光敏传感器最近一次原始值 */
static int32_t s_lightF;                         /* 光敏传感器滤波值 */
static int32_t s_gx_f;               /* 邮箱3：滤波后的 Gx（原始 LSB） */
static uint32_t s_lastTick = 0;
extern I2C_HandleTypeDef hi2c2;
extern ADC_HandleTypeDef hadc1;
void App_Init(void)
{
    Debug_Init(&huart1);
    s_mpuInitRc = MPU6050_Init(&hi2c2);
    s_lightRc = LightSensor_Init(&hadc1);
    MovingAvg_Init(&f_light);
    Debug_Print("Calibrating gyro, keep still...\r\n");
    s_gyroCalRc = MPU6050_CalibrateGyro(&hi2c2, 200);
    MovingAvg_Init(&f_gx);
    Debug_Print("MPU Init rc=");
    Debug_PrintInt((int32_t)s_mpuInitRc);
    Debug_Print("\r\n");

    Debug_Print("Gyro Cal rc=");
    Debug_PrintInt((int32_t)s_gyroCalRc);
    Debug_Print("\r\n");

    Debug_Print("Light Init rc=");
    Debug_PrintInt((int32_t)s_lightRc);
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
        s_mpuReadCount++;

        s_sampleRc = MPU6050_ReadData(&hi2c2, &s_sample);

        if (s_sampleRc == HAL_OK)
        {
            s_gx_f = MovingAvg_Update(&f_gx, s_sample.gx);
        }
        else
        {
            s_mpuFailCount++;
            s_mpuLastFailRc = s_sampleRc;
        }

        s_lightRc = LightSensor_ReadRaw(&hadc1, &s_lightRaw);
        if (s_lightRc == HAL_OK)
        {
            s_lightF = MovingAvg_Update(&f_light, s_lightRaw);
        }
    }

    if (now - s_lastTick >= 1000u)
    {
        s_lastTick = now;
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

        if (s_sampleRc == HAL_OK)
        {
            /* 物理量换算：数据全部来自 100ms 层的邮箱 */
            int32_t ax = (int32_t)s_sample.ax * 100 / 16384;
            int32_t ay = (int32_t)s_sample.ay * 100 / 16384;
            int32_t az = (int32_t)s_sample.az * 100 / 16384;
            int32_t gx = (int32_t)s_sample.gx * 100 / 131;
            int32_t gy = (int32_t)s_sample.gy * 100 / 131;
            int32_t gz = (int32_t)s_sample.gz * 100 / 131;
            int32_t gx_f = (int32_t)s_gx_f * 100 / 131;
            Debug_Print("AX="); Debug_PrintFixed(ax, 2);
            Debug_Print(" AY="); Debug_PrintFixed(ay, 2);
            Debug_Print(" AZ="); Debug_PrintFixed(az, 2);
            Debug_Print(" | Gx="); Debug_PrintFixed(gx, 1);
            Debug_Print(" Gx_F="); Debug_PrintFixed(gx_f, 1);
            Debug_Print(" Gy="); Debug_PrintFixed(gy, 1);
            Debug_Print(" Gz="); Debug_PrintFixed(gz, 1);
            OLED_ShowString(2, 1, "AX=");
            OLED_ShowSignedNum(2, 4, s_sample.ax, 6);
            OLED_ShowString(3, 1, "Gx=");
            OLED_ShowSignedNum(3, 4, s_sample.gx, 6);
        }
        else
        {
            Debug_Print("FAIL rc="); Debug_PrintInt((int32_t)s_sampleRc);
        }

        if (s_lightRc == HAL_OK)
        {
            Debug_Print(" LightRaw="); Debug_PrintInt((int32_t)s_lightRaw);
            Debug_Print(" LightF="); Debug_PrintInt(s_lightF);
        }
        else
        {
            Debug_Print(" Light_FAIL rc="); Debug_PrintInt((int32_t)s_lightRc);
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
}
