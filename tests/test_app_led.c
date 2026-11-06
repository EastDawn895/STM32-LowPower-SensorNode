#include <assert.h>
#include "main.h"
#include "app.h"
#include "sensor_manager.h"
#include "light_state.h"
#include "motion_state.h"
#include "data_logger.h"
#include "debug.h"
#include "oled.h"

/* Real App and both FSMs; only peripherals/services are replaced. */
UART_HandleTypeDef huart1;
static uint32_t tick;
static unsigned toggles, sleeps;
static LoggerStatus_t logger;
static HAL_StatusTypeDef oled, uart;
uint32_t HAL_GetTick(void) { return tick; }
void HAL_GPIO_TogglePin(int port, int pin)
{
    assert(port == LED_GPIO_Port && pin == LED_Pin);
    toggles++;
}
void HAL_PWR_EnterSLEEPMode(int regulator, int entry)
{
    assert(regulator == PWR_MAINREGULATOR_ON && entry == PWR_SLEEPENTRY_WFI);
    sleeps++;
}
void SensorManager_Init(void)
{
    LightState_Init();
    MotionState_Init();
    g_sensorData.mpuValid = g_sensorData.lightValid = 1;
}
void SensorManager_Service(void) {}
void SensorManager_Update(void) {}
HAL_StatusTypeDef W25Q64_Init(void) { return HAL_OK; }
HAL_StatusTypeDef DataLogger_Init(void) { logger = LOGGER_READY; return HAL_OK; }
LoggerStatus_t DataLogger_GetStatus(void) { return logger; }
void DataLogger_Update(void) {}
void Monitor_Init(void) {}
void Monitor_Update(void) {}
void OLED_Service(void) {}
HAL_StatusTypeDef OLED_GetStatus(void) { return oled; }
void Debug_Init(UART_HandleTypeDef *handle) { (void)handle; }
void Debug_BeginFrame(void) {}
void Debug_EndFrame(void) {}
void Debug_Print(const char *s) { (void)s; }
void Debug_PrintInt(int32_t value) { (void)value; }
HAL_StatusTypeDef Debug_GetStatus(void) { return uart; }

/* Establish a toggle, then check both sides of the selected deadline. */
static void interval(uint32_t ms)
{
    tick += 1000u;
    unsigned before = toggles;
    App_Run();
    assert(toggles == before + 1u);
    tick += ms - 1u;
    App_Run();
    assert(toggles == before + 1u);
    tick++;
    App_Run();
    assert(toggles == before + 2u);
}
static void light(int32_t value, LightState_t expected)
{
    LightState_Update(value);
    tick += LIGHT_STATE_CONFIRM_MS;
    LightState_Update(value);
    assert(LightState_Get() == expected);
}
int main(void)
{
    App_Init();
    interval(1000);
    light(1000, LIGHT_HIGH); interval(250);
    light(2500, LIGHT_NORMAL); interval(1000);
    light(3500, LIGHT_LOW); interval(250);
    light(2500, LIGHT_NORMAL); interval(1000);
    MotionState_Update(3000); tick += MOTION_CONFIRM_MS; MotionState_Update(3000);
    assert(MotionState_Get() == MOTION_ACTIVE); interval(250);
    MotionState_Update(0); tick += MOTION_CONFIRM_MS; MotionState_Update(0);
    assert(MotionState_Get() == MOTION_IDLE); interval(1000);
    /* Reproduce NORMAL/IDLE + Log=2: intended fault blink, no latch. */
    logger = LOGGER_FORMAT_ERROR; interval(100);
    light(1000, LIGHT_HIGH); interval(100);
    light(2500, LIGHT_NORMAL); interval(100);
    logger = LOGGER_IO_ERROR; interval(100);
    logger = LOGGER_READY; interval(1000);
    oled = HAL_ERROR; interval(100); oled = HAL_OK; interval(1000);
    uart = HAL_TIMEOUT; interval(100); uart = HAL_OK; interval(1000);
    g_sensorData.mpuValid = 0; interval(100);
    g_sensorData.mpuValid = 1; interval(1000);
    g_sensorData.lightValid = 0; interval(100);
    g_sensorData.lightValid = 1; interval(1000);
    tick = UINT32_MAX - 500u; interval(1000);
    assert(sleeps == 63u);
    return 0;
}
