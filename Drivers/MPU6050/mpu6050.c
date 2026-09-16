//
// Created by 26092 on 2026/9/15.
//
#include "mpu6050.h"

HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t val = 0x00;

    /* ① 唤醒 */
    if (HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1,
                          I2C_MEMADD_SIZE_8BIT, &val, 1, 100) != HAL_OK)
        return HAL_ERROR;

    /* ② 采样率 */
    val = 0x00;
    if (HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, MPU6050_REG_SMPLRT_DIV,
                          I2C_MEMADD_SIZE_8BIT, &val, 1, 100) != HAL_OK)
        return HAL_ERROR;

    /* ③ 陀螺量程 ±250 */
    val = 0x00;
    if (HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, MPU6050_REG_GYRO_CONFIG,
                          I2C_MEMADD_SIZE_8BIT, &val, 1, 100) != HAL_OK)
        return HAL_ERROR;

    /* ④ 加速度量程 ±2g */
    val = 0x00;
    if (HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, MPU6050_REG_ACCEL_CONFIG,
                          I2C_MEMADD_SIZE_8BIT, &val, 1, 100) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
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

HAL_StatusTypeDef MPU6050_ReadData(I2C_HandleTypeDef *hi2c, MPU6050_Data_t *data)
{
    uint8_t buf[14];

    /* 一次读 14 字节：从 0x3B 开始，芯片自动递增地址 */
    if (HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H,
                         I2C_MEMADD_SIZE_8BIT, buf, 14, 100) != HAL_OK)
    {
        return HAL_ERROR;   /* 失败返回错误码 */
    }

    /* 大端拼接：高字节 << 8 | 低字节 */
    data->ax   = (int16_t)((buf[0]  << 8) | buf[1]);
    data->ay   = (int16_t)((buf[2]  << 8) | buf[3]);
    data->az   = (int16_t)((buf[4]  << 8) | buf[5]);
    data->temp = (int16_t)((buf[6]  << 8) | buf[7]);
    data->gx   = (int16_t)((buf[8]  << 8) | buf[9]);
    data->gy   = (int16_t)((buf[10] << 8) | buf[11]);
    data->gz   = (int16_t)((buf[12] << 8) | buf[13]);

    return HAL_OK;
}


