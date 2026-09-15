#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f1xx_hal.h"

/* MPU6050 I2C 从机地址（AD0 接地） */
#define MPU6050_ADDR         (0x68u << 1)

/* 关键寄存器 */
#define MPU6050_REG_WHO_AM_I 0x75u

/* 函数声明 */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c);
uint8_t MPU6050_ReadWhoAmI(I2C_HandleTypeDef *hi2c);

#endif /* MPU6050_H */
