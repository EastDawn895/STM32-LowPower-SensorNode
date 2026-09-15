//
// Created by 26092 on 2026/9/15.
//
#include "mpu6050.h"

HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c)
{



}


uint8_t MPU6050_ReadWhoAmI(I2C_HandleTypeDef *hi2c)
{
    uint8_t who = 0;

    if (HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, MPU6050_REG_WHO_AM_I,
                         I2C_MEMADD_SIZE_8BIT, &who, 1, 100) != HAL_OK)
    {
        return 0;   /* 读失败，返回 0 */
    }
    return who;     /* 读成功，返回真正的数据 */
}
