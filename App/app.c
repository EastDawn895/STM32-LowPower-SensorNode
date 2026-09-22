#include "app.h"
#include "debug.h"
#include "main.h"
#include "mpu6050.h"
#include "oled.h"
#include "moving_avg.h"              /* 新增 include */

static uint32_t s_sensorTick = 0;    /* 100ms 节拍自己的时基 */
static MovingAvg_t f_gx;             /* Gx 的滤波器实例 */
static MPU6050_Data_t s_sample;      /* 邮箱1：100ms 写，1Hz 读 */
static HAL_StatusTypeDef s_sampleRc; /* 邮箱2：最近一次读取成败 */
static int32_t s_gx_f;               /* 邮箱3：滤波后的 Gx（原始 LSB） */
static uint32_t s_lastTick = 0;
extern I2C_HandleTypeDef hi2c2;
void App_Init(void)
{
    Debug_Init(&huart1);
    MPU6050_Init(&hi2c2);
    Debug_Print("Calibrating gyro, keep still...\r\n");
    MPU6050_CalibrateGyro(&hi2c2, 200);
    MovingAvg_Init(&f_gx);
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
        s_sampleRc = MPU6050_ReadData(&hi2c2, &s_sample);
        if (s_sampleRc == HAL_OK) {
            s_gx_f = MovingAvg_Update(&f_gx, s_sample.gx);
        }
        /* 读取失败：故意什么都不做。坏数据不进窗口，
           s_sample 里保留的是上一份好数据，1Hz 层照常显示 */
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
            Debug_Print("\r\n");
            OLED_ShowString(2, 1, "AX=");
            OLED_ShowSignedNum(2, 4, s_sample.ax, 6);
            OLED_ShowString(3, 1, "Gx=");
            OLED_ShowSignedNum(3, 4, s_sample.gx, 6);
        }
        else
        {
            Debug_Print("FAIL rc="); Debug_PrintInt((int32_t)s_sampleRc);
            Debug_Print("\r\n");
        }
    }
}
