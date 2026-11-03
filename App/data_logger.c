#include "data_logger.h"

#include "debug.h"
#include "w25q64.h"
#include "main.h" // 包含 HAL_GetTick() 等
#include "sensor_state.h"

// 历史状态追踪，用于边沿触发
static MotionState_t LastMotionState = MOTION_IDLE;
static LightState_t LastLightState = LIGHT_NORMAL;
static uint32_t s_writeAddr = 0x000000;

void DataLogger_Init(void)
{
    // 负责读取 Flash 寻找下一个空白地址
    SensorLog_t  temp={0};
    for (s_writeAddr=0;s_writeAddr<8192;s_writeAddr+=32)
    {
        W25Q64_ReadData(s_writeAddr, (uint8_t*)&temp,sizeof(SensorLog_t));
        if (temp.Log_Time == 0xFFFFFFFF)break;
    }
    if (s_writeAddr >= 8192)s_writeAddr = 0;

    Debug_Print("Init s_writeAddr:");
    Debug_PrintInt(s_writeAddr);
}


static void DataLogger_WriteRecord(uint8_t source, uint8_t state, int32_t value)
{
    if (s_writeAddr % 4096==0)W25Q64_EraseSector(s_writeAddr);
    SensorLog_t log = {0};
    log.Log_Source = source;
    log.Log_LState = state;
    log.Log_Value = value;
    log.Log_Time =HAL_GetTick();
    W25Q64_PageProgram(s_writeAddr, (uint8_t*)&log,sizeof(SensorLog_t));

    SensorLog_t readBack={0};
    W25Q64_ReadData(s_writeAddr, (uint8_t*)&readBack,sizeof(SensorLog_t));
    /*用 Debug_Print 和 Debug_PrintInt 把 readBack.Log_Time、readBack.Log_Source、
     *readBack.Log_LState 和 readBack.Log_Value 依次打印到串口上（顺便还可以打印一下当
     *前的 s_writeAddr 看看地址是不是每次加 32）。*/

    Debug_Print("readBack_Time:");
    Debug_PrintInt(readBack.Log_Time);
    Debug_Print(" Source:");
    Debug_PrintInt(readBack.Log_Source);
    Debug_Print(" LState:");
    Debug_PrintInt(readBack.Log_LState);
    Debug_Print(" Value:");
    Debug_PrintInt(readBack.Log_Value);
    Debug_Print(" s_waddr:");
    Debug_PrintInt(s_writeAddr);
    Debug_Print("\r\n");
    s_writeAddr += sizeof(SensorLog_t);
    s_writeAddr %= 8192;
}


void DataLogger_Update(void)
{
    MotionState_t currentMotion = MotionState_Get();
    LightState_t currentLight = LightState_Get();
    // 边沿检测：状态发生改变的瞬间
    if (currentMotion != LastMotionState)
    {

        if (currentMotion == MOTION_ACTIVE)
        {
            // TODO: 构建 SensorLog_t，计算 Flash 地址，写入日志
            Debug_Print("Logger: Motion Triggered!\r\n");
            DataLogger_WriteRecord(1,currentMotion,g_sensorData.gxFiltered );
        }

        LastMotionState = currentMotion; // 更新历史状态，吃掉变化
    }

    // 光照传感器的边沿检测逻辑也可以按此格式添加...

    if (currentLight != LastLightState)
    {
        if (currentLight != LIGHT_NORMAL)
        {
            Debug_Print("Logger: Light Triggered!\r\n");
            DataLogger_WriteRecord(2,currentLight,g_sensorData.lightFiltered );
        }
        LastLightState = currentLight;
    }

}
