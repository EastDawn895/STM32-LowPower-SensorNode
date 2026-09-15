#include "app.h"
#include "debug.h"
#include "main.h"

static uint32_t s_lastTick = 0;

void App_Init(void)
{
    Debug_Init(&huart1);
    Debug_Print("System Init OK\r\n");
}

void App_Run(void)
{
    uint32_t now = HAL_GetTick();

    if (now - s_lastTick >= 1000u)   /* ① 无符号减法，天然防回绕 */
    {
        s_lastTick = now;             /* ② 用 now 更新，不是 s_lastTick += 1000 */

        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        Debug_Print("Tick 1s\r\n");
    }
}
