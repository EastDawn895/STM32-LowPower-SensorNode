#include "app.h"
#include "debug.h"
#include "main.h"
#include "mpu6050.h"

static uint32_t s_lastTick = 0;
extern I2C_HandleTypeDef hi2c2;
void App_Init(void)
{
    Debug_Init(&huart1);
    MPU6050_Init(&hi2c2);
    Debug_Print("System Init OK\r\n");
    uint8_t who = MPU6050_ReadWhoAmI(&hi2c2);
    Debug_Print("WHO_AM_I = ");
    Debug_PrintHex(who);
    Debug_Print("\r\n");


}

void App_Run(void)
{
    uint32_t now = HAL_GetTick();
    MPU6050_Data_t data;   /* 每次读的数据 */

    if (now - s_lastTick >= 1000u)
    {
        s_lastTick = now;
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

        if (MPU6050_ReadData(&hi2c2, &data) == HAL_OK)
        {
            /* 这里用 Debug_Print + Debug_PrintInt 打印 6 个轴的值 */
            if (MPU6050_ReadData(&hi2c2, &data) == HAL_OK)
            {
                Debug_Print("AX=");  Debug_PrintInt(data.ax);
                Debug_Print(" AY="); Debug_PrintInt(data.ay);
                Debug_Print(" AZ="); Debug_PrintInt(data.az);
                Debug_Print(" GX="); Debug_PrintInt(data.gx);
                Debug_Print(" GY="); Debug_PrintInt(data.gy);
                Debug_Print(" GZ="); Debug_PrintInt(data.gz);
                Debug_Print("\r\n");
            }
            else
            {
                Debug_Print("ReadData FAIL\r\n");
            }
        }
        else
        {
            Debug_Print("ReadData FAIL\r\n");
        }
    }
}
