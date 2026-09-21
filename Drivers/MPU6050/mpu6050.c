//
// Created by 26092 on 2026/9/15.
//
#include "mpu6050.h"

static int32_t s_gyro_bias[3];

HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c) {
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


uint8_t MPU6050_ReadWhoAmI(I2C_HandleTypeDef *hi2c) {
    uint8_t who = 0;

    if (HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, MPU6050_REG_WHO_AM_I,
                         I2C_MEMADD_SIZE_8BIT, &who, 1, 100) != HAL_OK) {
        return 0; /* 读失败，返回 0 */
    }
    return who; /* 读成功，返回真正的数据 */
}

HAL_StatusTypeDef MPU6050_ReadData(I2C_HandleTypeDef *hi2c, MPU6050_Data_t *data) {
    uint8_t buf[14];
    HAL_StatusTypeDef status;

    /* 一次读 14 字节：从 0x3B 开始，芯片自动递增地址 */
    status = HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H,
                              I2C_MEMADD_SIZE_8BIT, buf, 14, 100);
    if (status != HAL_OK) {
        return status; /* 把原始返回值原样传出去，别吞成 HAL_ERROR */
    }

    /* 大端拼接：高字节 << 8 | 低字节 */
    data->ax = (int16_t) ((buf[0] << 8) | buf[1]);
    data->ay = (int16_t) ((buf[2] << 8) | buf[3]);
    data->az = (int16_t) ((buf[4] << 8) | buf[5]);
    data->temp = (int16_t) ((buf[6] << 8) | buf[7]);
    /* 陀螺三轴减去开机校准出的零偏：int32 中转再做减法，防溢出 int16 */
    data->gx = (int16_t) ((int32_t)((buf[8] << 8) | buf[9]) - s_gyro_bias[0]);
    data->gy = (int16_t) ((int32_t)((buf[10] << 8) | buf[11]) - s_gyro_bias[1]);
    data->gz = (int16_t) ((int32_t)((buf[12] << 8) | buf[13]) - s_gyro_bias[2]);

    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_CalibrateGyro(I2C_HandleTypeDef *hi2c, uint16_t samples) {
    /* 步骤：
       ① samples == 0 → return HAL_ERROR（防除零）
       ② 定义 MPU6050_Data_t 和三个 int32_t 累加器，清零
       ③ 循环 samples 次：ReadData 读一份 → gx/gy/gz 分别累加 → HAL_Delay(2)
       ④ 循环外：累加器各除以 samples，存入 s_gyro_bias[0..2]
       ⑤ 任何一次 ReadData 失败 → 原样 return 它的错误码
       全部成功 → return HAL_OK */
    if (samples == 0)return HAL_ERROR;

    MPU6050_Data_t d;
    int32_t sum_x = 0, sum_y = 0, sum_z = 0;
    uint16_t i;

    for (i = 0; i < samples; i++) {
        HAL_StatusTypeDef rc = MPU6050_ReadData(hi2c, &d);
        if (rc != HAL_OK) return rc;   /* 原样传出去 */
        sum_x += d.gx;
        sum_y += d.gy;
        sum_z += d.gz;
        HAL_Delay(2);
    }
    s_gyro_bias[0] = sum_x / samples;
    s_gyro_bias[1] = sum_y / samples;
    s_gyro_bias[2] = sum_z / samples;

    return HAL_OK;
}
