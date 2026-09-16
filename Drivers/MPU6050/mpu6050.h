#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f1xx_hal.h"

/* I2C 从机地址（AD0 接地） */
#define MPU6050_ADDR            (0x68u << 1)

/* 配置寄存器 */
#define MPU6050_REG_SMPLRT_DIV   0x19u
#define MPU6050_REG_CONFIG       0x1Au
#define MPU6050_REG_GYRO_CONFIG  0x1Bu
#define MPU6050_REG_ACCEL_CONFIG 0x1Cu

/* 数据寄存器（0x3B 起连续 14 字节） */
#define MPU6050_REG_ACCEL_XOUT_H 0x3Bu
#define MPU6050_REG_ACCEL_XOUT_L 0x3Cu
#define MPU6050_REG_ACCEL_YOUT_H 0x3Du
#define MPU6050_REG_ACCEL_YOUT_L 0x3Eu
#define MPU6050_REG_ACCEL_ZOUT_H 0x3Fu
#define MPU6050_REG_ACCEL_ZOUT_L 0x40u
#define MPU6050_REG_TEMP_OUT_H   0x41u
#define MPU6050_REG_TEMP_OUT_L   0x42u
#define MPU6050_REG_GYRO_XOUT_H  0x43u
#define MPU6050_REG_GYRO_XOUT_L  0x44u
#define MPU6050_REG_GYRO_YOUT_H  0x45u
#define MPU6050_REG_GYRO_YOUT_L  0x46u
#define MPU6050_REG_GYRO_ZOUT_H  0x47u
#define MPU6050_REG_GYRO_ZOUT_L  0x48u

/* 电源管理寄存器 */
#define MPU6050_REG_PWR_MGMT_1   0x6Bu
#define MPU6050_REG_PWR_MGMT_2   0x6Cu
#define MPU6050_REG_WHO_AM_I     0x75u

/* 公共接口 */
typedef struct {
    int16_t ax, ay, az;
    int16_t temp;
    int16_t gx, gy, gz;
} MPU6050_Data_t;

HAL_StatusTypeDef MPU6050_ReadData(I2C_HandleTypeDef *hi2c, MPU6050_Data_t *data);
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c);
uint8_t MPU6050_ReadWhoAmI(I2C_HandleTypeDef *hi2c);







#endif /* MPU6050_H */
