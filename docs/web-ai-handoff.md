# STM32-LowPower-SensorNode Web AI Handoff

更新时间：2026-09-28

## 1. 交接目标

你现在接手的是一个正在实际开发中的 STM32 嵌入式项目，不是一个从零开始的教学示例。

项目目标：

- 基于 STM32F103C8T6 构建低功耗实时数据采集与监控节点
- 完成多传感器采集、数据处理、异常判断、报警、数据存储和低功耗
- 最终整理成可以放到 GitHub、用于嵌入式实习展示的工程项目

请使用中文交流。README 和公开项目文档使用英文。

## 2. 硬件与工具链

| 项目 | 当前配置 |
| --- | --- |
| MCU | STM32F103C8T6，Cortex-M3，无 FPU，64 KB Flash / 20 KB RAM |
| 开发板 | Blue Pill |
| IDE | CLion |
| 配置工具 | STM32CubeMX |
| 库 | STM32 HAL |
| 构建 | CMake + Ninja |
| 编译器 | arm-none-eabi-gcc |
| 系统时钟 | 8 MHz HSE + PLL x9 = 72 MHz |
| APB1 / APB2 | 36 MHz / 72 MHz |
| 调试串口 | USART1，PA9 TX / PA10 RX，115200 8N1 |
| MPU6050 | I2C2，PB10 SCL / PB11 SDA，AD0 接地，地址 0x68 |
| 光敏传感器 | ADC1 Channel 0，PA0 |
| LED | PA6 |
| OLED | 软件 I2C，当前驱动使用 PB8 / PB9 |

HAL I2C 的 MPU6050 地址使用：

```c
#define MPU6050_ADDR (0x68u << 1)
```

不能把旧 SPL 中的 `0xD0` 直接复制到 HAL API。

## 3. 当前目录结构

```text
STM32-LowPower-SensorNode/
├── App/
│   ├── app.c
│   └── app.h
├── Core/
│   ├── Inc/
│   └── Src/
├── Drivers/
│   ├── Debug/
│   ├── LightSensor/
│   ├── MPU6050/
│   ├── OLED/
│   ├── CMSIS/
│   ├── STM32F1xx_HAL_Driver/
│   └── ...
├── Libs/
│   └── Filter/
├── cmake/
├── docs/
├── CMakeLists.txt
├── CMakePresets.json
├── STM32-LowPower-SensorNode.ioc
└── STM32F103xx_FLASH.ld
```

分层原则：

```text
CubeMX/Core
    ↓
Drivers
    ↓
Libs/Middleware
    ↓
App
```

用户业务代码优先放在 `App/`、`Drivers/` 和 `Libs/`，不要把业务逻辑堆到 `main.c`。

## 4. 已完成的功能

- HAL + CubeMX + CMake + GCC 工程骨架
- 72 MHz 系统时钟
- `App_Init()` / `App_Run()` 应用框架
- `HAL_GetTick()` 非阻塞调度
- PA6 LED 心跳
- USART1 调试输出
- MPU6050 初始化
- `WHO_AM_I` 读取
- 0x3B 起始的 14 字节连续读取
- 加速度、温度、陀螺仪原始数据解析
- 陀螺仪开机零偏校准
- Gx 滑动平均滤波
- SSD1306 OLED 软件 I2C 驱动
- ADC1 PA0 单通道采样
- 光敏传感器原始值读取
- 光敏传感器滑动平均滤波

## 5. 非常重要的技术约束

### 5.1 禁止 printf 和浮点格式化

STM32F103C8T6 没有 FPU，Flash 空间有限。项目禁止使用：

- `printf`
- `sprintf`
- `snprintf`
- `%f`
- `-u _printf_float`

当前使用 `Drivers/Debug` 中的纯整数接口：

```c
Debug_Init(&huart1);
Debug_Print("text");
Debug_PrintInt(value);
Debug_PrintHex(value);
Debug_PrintFixed(value, decimals);
```

物理量使用定点数表达：

```c
int32_t ax = (int32_t)raw_ax * 100 / 16384;
Debug_PrintFixed(ax, 2);
```

### 5.2 HAL 返回值和数据必须分开

例如：

```c
uint8_t data;
HAL_StatusTypeDef status;

status = HAL_I2C_Mem_Read(
    hi2c,
    MPU6050_ADDR,
    register_address,
    I2C_MEMADD_SIZE_8BIT,
    &data,
    1,
    100
);
```

`HAL_I2C_Mem_Read()` 返回的是 `HAL_OK`、`HAL_ERROR`、`HAL_BUSY` 或 `HAL_TIMEOUT`，真正的数据通过指针返回。

ADC 也遵循同样原则：

```c
HAL_StatusTypeDef LightSensor_ReadRaw(
    ADC_HandleTypeDef *hadc,
    uint16_t *raw
);
```

### 5.3 非阻塞业务调度

实际业务不要使用大量 `HAL_Delay()`。

当前调度模型：

- MPU6050 和光敏传感器：100 ms
- 串口输出和 OLED 更新：1000 ms
- 初始化阶段的校准可以暂时使用 `HAL_Delay()`

调度必须保留时间戳更新：

```c
if (now - s_sensorTick >= 100u)
{
    s_sensorTick = now;
    /* 执行一次采样 */
}
```

删除 `s_sensorTick = now` 会导致条件一直成立，使采样变成主循环高速连续执行。

### 5.4 CubeMX 代码边界

原则上由 `.ioc` 管理：

- `SystemClock_Config()`
- `MX_GPIO_Init()`
- `MX_USART1_UART_Init()`
- `MX_I2C2_Init()`
- `MX_ADC1_Init()`
- MSP 初始化

业务代码放在：

- `App/`
- `Drivers/`
- `Libs/`
- `USER CODE` 区域

不要随意删除用户已有修改，也不要使用 `git reset --hard` 或 `git checkout --` 覆盖工作区。

## 6. 当前关键代码状态

### 6.1 `App/app.c`

当前应用层包含：

```c
App_Init();
App_Run();
```

`App_Run()` 中，100 ms 任务应包含：

```c
s_sensorTick = now;

s_sampleRc = MPU6050_ReadData(&hi2c2, &s_sample);

s_lightRc = LightSensor_ReadRaw(&hadc1, &s_lightRaw);
if (s_lightRc == HAL_OK)
{
    s_lightF = MovingAvg_Update(&f_light, s_lightRaw);
}
```

1 秒任务输出 MPU6050 和光敏传感器数据。

当前还保留了以下 MPU 诊断变量：

```c
static uint32_t s_mpuReadCount;
static uint32_t s_mpuFailCount;
static HAL_StatusTypeDef s_mpuLastFailRc;
```

它们目前已经统计，但还没有完整显示在串口日志中。后续应输出每秒采样次数和失败次数，用于确认 I2C 是否真正稳定。

### 6.2 光敏传感器

文件：

- `Drivers/LightSensor/light_sensor.c`
- `Drivers/LightSensor/light_sensor.h`

初始化：

```c
LightSensor_Init(&hadc1);
```

读取：

```c
LightSensor_ReadRaw(&hadc1, &s_lightRaw);
```

当前 ADC 配置：

- ADC1
- Channel 0
- PA0
- 软件触发
- 单次转换
- 采样时间 71.5 cycles
- ADC 时钟 12 MHz

### 6.3 MPU6050

文件：

- `Drivers/MPU6050/mpu6050.c`
- `Drivers/MPU6050/mpu6050.h`

已实现：

- `MPU6050_Init()`
- `MPU6050_ReadWhoAmI()`
- `MPU6050_ReadData()`
- `MPU6050_CalibrateGyro()`

初始化量程：

- 加速度：±2g，16384 LSB/g
- 陀螺仪：±250 dps，131 LSB/(degree/s)

数据寄存器从 `0x3B` 开始，连续读取 14 字节。

## 7. 最近一次实际运行结果

MPU6050 输出曾经稳定为：

```text
WHO_AM_I = 0x68
AX approximately -0.04
AY approximately -0.04
AZ approximately 1.06
Gx/Gy/Gz mostly close to 0
```

这说明 MPU6050 初始化、I2C 地址、14 字节解析和陀螺仪校准基本正常。

光敏传感器之前曾经读到：

```text
LightRaw=1106 ~ 3659
```

后来出现连续：

```text
LightRaw=0 LightF=0
```

代码检查后发现当时 `App_Run()` 中漏掉了 `LightSensor_ReadRaw()` 调用，同时也漏掉了 `s_sensorTick = now`。

这两个问题已经修复，最近一次 Debug 构建成功。必须重新烧录修复后的 ELF 后再验证硬件。

## 8. 当前 Git 状态

当前分支：

```text
main
```

HEAD 最近提交：

```text
a061efd refactor: move filter module to Libs/Filter
0418dc3 feat: dual-rate scheduler with gyro filtering (10Hz sample / 1Hz display)
a452185 fix: use -Og optimization for Debug builds to fit 64KB flash
c4239be feat: add moving-average filter with host-side unit tests
24dd441 feat: add SSD1306 OLED module and engineering-unit output
8b7a845 feat: add MPU6050 init and data reading
```

当前工作区有未提交改动，主要包括：

- ADC1 和光敏传感器接入
- `App/app.c` 调度与诊断代码
- CubeMX `.ioc` 配置变化
- HAL ADC 驱动文件
- `CMakeLists.txt` 和 CubeMX CMake 源文件
- EXTI/PB5 相关生成代码
- 滤波模块格式调整

不要清理或回滚这些改动。接手后应先审阅并整理，再按功能拆分提交。

## 9. 构建与验证

当前已有 Debug 构建目录时：

```bash
cmake --build cmake-build-debug --parallel 2
```

如果需要使用 CMake Preset：

```bash
cmake --preset Debug
cmake --build --preset Debug
```

检查固件大小：

```bash
arm-none-eabi-size cmake-build-debug/STM32-LowPower-SensorNode.elf
```

最近一次构建结果约为：

```text
text: 49572
data: 1376
bss:  2760
```

Flash 使用量约为 `text + data = 50948 bytes`，必须继续关注 64 KB Flash 限制。

## 10. 下一步开发顺序

不要立刻加入 FreeRTOS、W25Q64 或复杂算法。建议顺序如下：

1. 烧录最新 ELF，确认 `LightRaw` 恢复正常
2. 在串口输出每秒 MPU 读取次数、失败次数和最后错误码
3. 检查 `MPU6050_Init()`、`MPU6050_CalibrateGyro()`、`LightSensor_Init()` 的返回值
4. 整理 ADC/光敏传感器代码并提交一个独立 commit
5. 继续完善陀螺仪零偏校准和异常值处理
6. 根据传感器特性设计滤波策略
7. 设计 NORMAL / WARNING / ALARM 状态机
8. 加入阈值、恢复阈值和滞回
9. 再设计 W25Q64 数据记录格式和环形日志
10. 最后考虑 Sleep/Stop 低功耗和 FreeRTOS

## 11. 协作规则

网页端 AI 必须遵守：

1. 先读取实际仓库、代码、构建配置和错误输出，再提出判断。
2. 不要假设当前代码仍然和历史记录完全一致。
3. 调试使用：

```text
现象
→ 最可能原因
→ 验证方法
→ 验证结果
→ 下一步
```

4. 核心模块采用：

```text
设计接口
→ 解释关键决策
→ 让我实现关键部分
→ 编译
→ 烧录
→ 观察输出
→ 修正
```

5. 机械性的 HAL 封装、CubeMX 模板和重复性代码可以直接修改。
6. 不要一次生成几百行核心代码。
7. 不要使用 `printf`、浮点格式化或无意义的 HAL/DMA/RTOS 堆叠。
8. 不要删除、覆盖或回滚用户未提交的改动。
9. 每完成一个有意义阶段，使用 conventional commit：

```text
feat:
fix:
refactor:
test:
docs:
```

## 12. 给网页端 AI 的第一条指令

可以把下面这段和本文件一起发送给网页端 AI：

```text
你现在接手 STM32-LowPower-SensorNode 项目。

请先读取项目中的 docs/web-ai-handoff.md、App/app.c、Core/Src/main.c、STM32-LowPower-SensorNode.ioc、Drivers/LightSensor、Drivers/MPU6050、Drivers/Debug、Libs/Filter 和 CMakeLists.txt。

不要从 GPIO、UART 或 STM32 基础开始教学，也不要重写项目。
先检查当前工作区的未提交改动和实际构建状态。

当前最近修复：
1. App_Run() 的 100 ms 任务中恢复了 LightSensor_ReadRaw(&hadc1, &s_lightRaw)。
2. 恢复了 s_sensorTick = now，避免采样任务变成主循环高速连续执行。
3. Debug 构建已经成功，但最新固件还需要重新烧录验证 LightRaw。

接下来请按照“现象 → 最可能原因 → 验证方法 → 结果 → 下一步”的方式工作。
第一任务是确认最新固件烧录后：
- WHO_AM_I 是否为 0x68
- MPU6050 是否每秒约采样 10 次
- MPU I2C 是否有失败次数
- LightRaw 是否恢复为随光照变化的 ADC 值

不要在验证完成前引入 FreeRTOS、W25Q64 或复杂状态机。
```
