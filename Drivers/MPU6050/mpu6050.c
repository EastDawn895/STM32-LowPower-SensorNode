#include "mpu6050.h"
#include <limits.h>
static int32_t bias[3], sum[3];
static int16_t minimum[3], maximum[3];
static uint16_t calTarget, calCount;
static uint8_t raw[14];
#define IO_MS 10u

static int16_t signed16(const uint8_t* p)
{
    uint32_t n = ((uint32_t)p[0] << 8) | p[1];
    return (int16_t)(n >= 32768u ? (int32_t)n - 65536 : (int32_t)n);
}

static int16_t corrected(const uint8_t* p, int32_t offset)
{
    int32_t n = (int32_t)signed16(p) - offset;
    if (n > INT16_MAX) n = INT16_MAX;
    if (n < INT16_MIN) n = INT16_MIN;
    return (int16_t)n;
}

static HAL_StatusTypeDef readraw(I2C_HandleTypeDef* i2c)
{
    if (!i2c) return HAL_ERROR;
    return HAL_I2C_Mem_Read(i2c,MPU6050_ADDR,MPU6050_REG_ACCEL_XOUT_H,I2C_MEMADD_SIZE_8BIT, raw, 14,IO_MS);
}

HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef* i2c)
{
    if (!i2c) return HAL_ERROR;
    uint8_t who = 0;
    HAL_StatusTypeDef rc = HAL_I2C_Mem_Read(i2c,MPU6050_ADDR,MPU6050_REG_WHO_AM_I,I2C_MEMADD_SIZE_8BIT, &who, 1,IO_MS);
    if (rc != HAL_OK) return rc;
    if (who != 0x68u) return HAL_ERROR;
    /* Explicit DLPF (~5 Hz gyro), 100 Hz internal sample rate, +/-250 dps, +/-2g. */
    const uint8_t regs[] = {
        MPU6050_REG_PWR_MGMT_1,MPU6050_REG_CONFIG,MPU6050_REG_SMPLRT_DIV,MPU6050_REG_GYRO_CONFIG,
        MPU6050_REG_ACCEL_CONFIG
    };
    uint8_t values[] = {1, 6, 9, 0, 0};
    for (unsigned i = 0; i < sizeof regs; i++)
    {
        rc = HAL_I2C_Mem_Write(i2c,MPU6050_ADDR, regs[i],I2C_MEMADD_SIZE_8BIT, &values[i], 1,IO_MS);
        if (rc != HAL_OK) return rc;
    }
    return HAL_OK;
}

uint8_t MPU6050_ReadWhoAmI(I2C_HandleTypeDef* i2c)
{
    uint8_t who = 0;
    if (!i2c) return 0;
    if (HAL_I2C_Mem_Read(i2c,MPU6050_ADDR,MPU6050_REG_WHO_AM_I,I2C_MEMADD_SIZE_8BIT, &who, 1,IO_MS) != HAL_OK) return 0;
    return who;
}

HAL_StatusTypeDef MPU6050_ReadData(I2C_HandleTypeDef* i2c, MPU6050_Data_t* data)
{
    if (!data) return HAL_ERROR;
    HAL_StatusTypeDef rc = readraw(i2c);
    if (rc != HAL_OK) return rc;
    data->ax = signed16(raw);
    data->ay = signed16(raw + 2);
    data->az = signed16(raw + 4);
    data->temp = signed16(raw + 6);
    data->gx = corrected(raw + 8, bias[0]);
    data->gy = corrected(raw + 10, bias[1]);
    data->gz = corrected(raw + 12, bias[2]);
    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_CalibrateStart(uint16_t samples)
{
    if (!samples) return HAL_ERROR;
    calTarget = samples;
    calCount = 0;
    for (unsigned i = 0; i < 3; i++)
    {
        sum[i] = 0;
        minimum[i] = INT16_MAX;
        maximum[i] = INT16_MIN;
    }
    return HAL_OK;
}

/* One sample; caller spaces samples >=10 ms and keeps board stationary. */
HAL_StatusTypeDef MPU6050_CalibrateStep(I2C_HandleTypeDef* i2c)
{
    if (!calTarget) return HAL_ERROR;
    HAL_StatusTypeDef rc = readraw(i2c);
    if (rc != HAL_OK)
    {
        calTarget = 0;
        return rc;
    }
    for (unsigned i = 0; i < 3; i++)
    {
        int16_t n = signed16(raw + 8 + 2u * i);
        sum[i] += n;
        if (n < minimum[i])minimum[i] = n;
        if (n > maximum[i])maximum[i] = n;
        if ((int32_t)maximum[i] - minimum[i] > 500)
        {
            calTarget = 0;
            return HAL_ERROR;
        }
    }
    if (++calCount < calTarget) return HAL_BUSY;
    for (unsigned i = 0; i < 3; i++) bias[i] = sum[i] / calTarget;
    calTarget = 0;
    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_CalibrateGyro(I2C_HandleTypeDef* i2c, uint16_t samples)
{
    if (!i2c || MPU6050_CalibrateStart(samples) != HAL_OK) return HAL_ERROR;
    HAL_StatusTypeDef rc;
    do
    {
        rc = MPU6050_CalibrateStep(i2c);
        if (rc == HAL_BUSY) HAL_Delay(10);
    }
    while (rc == HAL_BUSY);
    return rc;
}
