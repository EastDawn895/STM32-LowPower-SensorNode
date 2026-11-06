#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H
#include "stm32f1xx_hal.h"

/* On-flash v1 is explicitly little endian, not the in-memory struct layout. */
typedef struct
{
    uint32_t Log_Time;
    int32_t Log_Value;
    uint8_t Log_Source, Log_LState;
    uint8_t Log_Arr[22];
} SensorLog_t;

typedef enum { LOGGER_READY, LOGGER_IO_ERROR, LOGGER_FORMAT_ERROR } LoggerStatus_t;

HAL_StatusTypeDef DataLogger_Init(void); /* Flash must already be initialized. No automatic format of unknown media. */
void DataLogger_Update(void); /* Call every loop; one bounded I/O stage per call. */
HAL_StatusTypeDef DataLogger_Enqueue(uint8_t source, uint8_t state, int32_t value);
LoggerStatus_t DataLogger_GetStatus(void);
uint32_t DataLogger_GetDropped(void);
uint8_t DataLogger_IsIdle(void);
#endif
