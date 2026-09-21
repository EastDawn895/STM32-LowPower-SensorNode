#include "app.h"
#include "debug.h"
#include "main.h"
#include "mpu6050.h"
#include "oled.h"

static uint32_t s_lastTick = 0;
extern I2C_HandleTypeDef hi2c2;
void App_Init(void)
{
    Debug_Init(&huart1);
    MPU6050_Init(&hi2c2);
    Debug_Print("Calibrating gyro, keep still...\r\n");
    MPU6050_CalibrateGyro(&hi2c2, 200);
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
    MPU6050_Data_t data;   /* 每次读的数据 */

    if (now - s_lastTick >= 1000u)
    {
        s_lastTick = now;
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

        HAL_StatusTypeDef rc = MPU6050_ReadData(&hi2c2, &data);
        if (rc == HAL_OK)
        {
            /* 这里用 Debug_Print + Debug_PrintFixed 打印 6 个轴的值 */
            int32_t ax = (int32_t)data.ax * 100 / 16384;
            int32_t ay = (int32_t)data.ay * 100 / 16384;
            int32_t az = (int32_t)data.az * 100 / 16384;
            int32_t gx = (int32_t)data.gx * 100 / 131;
            int32_t gy = (int32_t)data.gy * 100 / 131;
            int32_t gz = (int32_t)data.gz * 100 / 131;
            Debug_Print("AX="); Debug_PrintFixed(ax, 2);
            Debug_Print(" AY="); Debug_PrintFixed(ay, 2);
            Debug_Print(" AZ="); Debug_PrintFixed(az, 2);
            Debug_Print(" | Gx="); Debug_PrintFixed(gx, 1);
            Debug_Print(" Gy="); Debug_PrintFixed(gy, 1);
            Debug_Print(" Gz="); Debug_PrintFixed(gz, 1);
            Debug_Print("\r\n");
            OLED_ShowString(2, 1, "AX=");
            OLED_ShowSignedNum(2, 4, data.ax, 6);
            OLED_ShowString(3, 1, "Gx=");
            OLED_ShowSignedNum(3, 4, data.gx, 6);
        }
        else
        {
            Debug_Print("FAIL rc="); Debug_PrintInt((int32_t)rc);
            Debug_Print("\r\n");
        }
    }
}

