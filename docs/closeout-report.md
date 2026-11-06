# 工程修复交付报告

日期：2026-11-03。基线HEAD：`bc51bfd1b602e03d781ea188406b603c2b9da034`，分支main。修复尚未提交，未push、打Tag或烧录。**软件验证通过，不能替代板上验收。建议作为v1.0.0候选版本，而非已经完成硬件验收的正式版本。**

## 1. C01–C10状态

| 编号 | 软件状态 | 实际修改与证据 |
|---|---|---|
| C01 | 已修复 | 0x05状态读取、JEDEC EF4017检查、WEL检查、操作超时、CS清理、SPI错误传播；flash和app测试通过 |
| C02 | 已修复 | HIGH进入<1800、退出>2000，LOW保持3200/2800，500 ms；边界、恒定1900、噪声、取消、回绕及禁止直接跨态测试通过 |
| C03 | 已修复（软件模型） | 双扇区代际、记录序号、CRC、最后提交、读回及擦除验证；多圈、重启、65种轮转切断注入、CRC坏、未知格式、序号/代际回绕通过；真实掉电待验 |
| C04 | 已修复 | Debug/Release有效sections+GC、Os、map/HEX；新目录完整构建通过，没有扩大链接容量或删除核心功能 |
| C05 | 已修复 | GX/GXF的100倍定点数统一两位小数，串口标注dps；正负零与两端极值输出通过 |
| C06 | 已修复 | 固定小数宽度、0位小数、INT32_MIN、安全无符号幅值及非法小数位处理；精确字符串断言通过 |
| C07 | 已修复 | UART/OLED均显式INVALID/ERROR，覆盖旧显示；过期、错误、恢复专项通过 |
| C08 | 已修复 | 无效样本取消Pending并清空对应窗口；配置/校准期间无效；连续、间歇、长时间失败恢复测试通过 |
| C09 | 已修复 | 不丢弃>30000大信号；有符号解码后减偏置，边界饱和；±31000、INT16两端和校准偏置组合通过 |
| C10 | 已修复 | 正确工具链路径、真实HEX产物、可执行测试命令；移除DMA、远程报警、Tickless及未经测量节电指标等错误声明 |

## 2. 构建与资源

| 配置 | 独立全量构建 | Flash（text+data加载量） | 静态RAM（data+bss） | 链接RAM含预留和对齐 |
|---|---|---|---|---|
| Debug | PASS | 22,936 B / 65,536 B（35.00%） | 2,256 B | 3,800 B / 20,480 B（18.55%） |
| Release | PASS | 20,228 B / 65,536 B（30.87%） | 2,256 B | 3,800 B / 20,480 B（18.55%） |

两配置.data=20 B、实际.bss=2,236 B；链接heap/stack段含对齐为1,540 B，另有段间对齐。size汇总bss=3,776包含预留，不能当成全局变量占用。heap最低512 B、stack最低1,024 B，未测栈高水位。

工具：ARM GCC14.3.1（STM32CubeCLT1.22.0），CMake4.3.1，Ninja1.13.2，本机GCC15.2.0。Debug有效Og，Release有效Os，Cortex-M3/Thumb/soft ABI保持一致；37个编译条目的sections参数已检查，链接实际包含GC。nm确认未用的HAL_I2C_Mem_Read_DMA、HAL_I2C_Master_Seq_Transmit_DMA不再被链接，正常HAL_I2C_Mem_Read保留。

**告警不是零：每个固件配置仍有3条厂商HAL隐式转换告警**，位于stm32f1xx_hal_flash_ex.c:398、530及stm32f1xx_hal_spi.c:1346。没有修改或屏蔽这些告警。自有/生成应用C文件和主机测试启用Werror并通过；字模只补显式括号，原有字节序列已逐项比较不变。

## 3. 实际执行命令和结果

最终新建验证目录：`build/validation.W69s2J`。该目录为gitignored构建输出，不会作为源文件提交；完整日志在各配置的configure/build/test日志中。最后汇总日志另存`/tmp/node-final-validation.log`。

执行过：

```bash
bash tests/run_validation.sh
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Release
cmake --build --preset Release
cmake -S . -B build/readme-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build/readme-debug
cmake -S tests -B build/host-analyzer -G Ninja -DCMAKE_C_FLAGS=-fanalyzer
cmake --build build/host-analyzer
ctest --test-dir build/host-analyzer --output-on-failure
git diff --check
```

- 完整脚本：Debug/Release各新目录38步构建PASS，ELF/HEX/map均存在。
- 主机普通配置：10/10 PASS。
- 主机Release优化配置：10/10 PASS，断言没有随NDEBUG关闭。
- ASan+UBSan Debug配置：10/10 PASS，未报告运行期内存/未定义行为错误。
- GCC fanalyzer主机配置：编译无告警，10/10 PASS。分析范围是真实模块+主机HAL桩，不包括真实寄存器运行或全厂商库。
- README预设命令及首次独立路径命令：PASS。
- HEX校验：两配置PASS，校验每条记录checksum、范围、结束及入口地址，所有数据字节与ELF32 PT_LOAD物理加载段一致。
- diff检查：PASS。HEAD和暂存区未变；所有修改保留为工作区diff/未跟踪新增文件，待用户审查。

## 4. 新增测试

10个CTest目标编译真实模块，不以重新实现生产算法替代它们：

| 测试 | 范围 |
|---|---|
| filter | 预热、满窗阶跃、长时间索引和int32输入边界 |
| debug | 固定小数、符号、0/整数、INT32_MIN、UINT32_MAX、无效参数、UART失败/总期限 |
| fsm | 阈值等于/±1、恒定及噪声、确认取消、两FSM、tick回绕、无直接LOW/HIGH切换 |
| flash | 协议、WEL、无器件、FF/错误ID、SPI失败/超时、读/编程负载失败、BUSY期限及CS释放 |
| logger | NOR的1→0/页/扇区约束，多圈及381/384条附近重启，65组分阶段切断，坏CRC、未知格式、队列满及序号/代际回绕 |
| sensors | 真实Manager+MPU驱动，配置/增量校准、±31000、失联及恢复、无效样本连续/间歇/长时间情况、偏置饱和及身份检查 |
| oled | 真实软件I2C+字模，ACK阶段释放SDA、NACK、SCL卡低、有界返回、重试及绘图参数边界 |
| monitor | 真实Monitor+Debug，精确单位值，错误/恢复覆盖OLED旧内容，以及过期数据 |
| adc | 真实ADC驱动的校准失败、转换失败/清理、停止失败、指针及4095值 |
| app | 真实App+Flash+Logger+Debug，无Flash时仍调用采样、监控与Sleep，不擦除/编程 |

另有`verify_hex.py`与`run_validation.sh`。HAL桩不模拟全部外设寄存器、电气和异常行为。

## 5. 工程风险处置

- **必须修复、已实施**：Flash错误链与无限等待、日志持久化协议、数据有效性与错误显示、错误物理量、合法大数及整数边界。
- **建议修复、本次已实施**：OLED ACK/明确DWT时序与分块刷新；UART刷新总预算；MPU身份、显式DLPF/采样配置、增量校准及冷却重试；ADC失败后重新校准；PB5普通输入下拉并同步IOC/中断；关键API边界；有效链接裁剪和主机测试。
- **保留并公开的限制**：无自动看门狗；致命Error_Handler/CPU异常维持fail-stop，不在停SysTick后调用依赖tick的诊断等待。真实总线卡死仍可能需要人工复位/修复接线；没有宣称任意硬件故障都能自动恢复。
- **时间限制**：SPI事务10 ms、Logger内部异步轮询；兼容同步擦除500 ms/编程10 ms并未用于Logger运行路径。UART帧20 ms；OLED一个包5 ms、Service最多两个包。MPU超时参数10 ms，但厂商HAL BUSY等待固定25 ms，不能据此宣称单次HAL调用硬上限10 ms。整机最坏调度延迟未实测。
- **参数/架构边界**：保留裸机框架、MCU及业务引脚；没有新增RTOS或远程功能。OLED新增1 KiB帧缓冲是为解决长时间同步显示阻塞，不是新增显示功能；Logger只在原模块内实现小型写入状态机及固定队列。

## 6. 兼容性与数据安全

Flash读写接口现返回状态，保留主要函数名/参数并更新仓库调用；保留ReadID兼容包装，可靠路径使用checked接口。现有外部函数指针绑定若存在需重新编译核对。新增服务函数由App主循环调用。

**旧Flash日志与v1不兼容。未知格式默认保护，不自动擦除。** 若现板仍有旧日志，新固件会报告格式错误而不是默默初始化；需要用户先备份并自主决定是否清除8 KiB日志区。本任务没有访问或擦除硬件。新格式容量254槽，擦除时会淘汰整个旧扇区；不是始终保留254条。

CRC+提交只建立模型内的完整性判定，不保证RAM中尚未提交事件在断电后存在；失败时Drop表示“拒收或未确认持久化”，可能包含已经在器件上完成但未能确认的记录。详见log-format.md。

## 7. README和工程说明

README重写为当前实现：普通WFI Sleep、保留SysTick；无DMA业务、无Stop/Standby/Tickless、无远程报警；不写功耗百分比。说明裸机同步I/O边界、启动/恢复静止校准、状态阈值、单位、日志迁移、错误码、测试和构建命令。MPU笔记同步修正旧倍率和过时采样/校准描述。

## 8. 未解决项与硬件验证

没有剩余已知C01–C10软件修复项，但以下未完成，不能记为PASS：

| 项目 | 操作与判定 |
|---|---|
| 基础外设 | 检查8 MHz晶振/时钟、PA0已知电压、WHO_AM_I、JEDEC、UART115200及OLED接线，数据与实物一致 |
| 光照方向和状态 | 真实明暗/边界扰动，确认ADC方向和阈值物理含义，稳定输入不自激 |
| MPU静止/恢复 | 静止校准、运动启动、温漂、单独断电恢复；不得把错误校准当正常 |
| OLED波形 | Debug/Release下分析SCL/SDA、ACK、上升时间及DWT可用性；断线后UART/LED有错误表现 |
| Flash随机断电 | 头/记录主体与提交、扇区擦除时断电；确认保留的最新扇区、坏槽识别及重启续写 |
| 调度/可靠性 | GPIO或逻辑分析仪测最大采样间隔，包含擦除、通信错误及UI刷新；长时间运行和栈高水位 |
| 功耗 | 同条件整板测Sleep与忙循环平均电流；完成前不声明实测收益 |

主机结果不能证明电气质量、Flash耐久/物理掉电模式、实际采样误差、最大栈或整板功耗。SPI/HAL超时要求中断与SysTick正常工作。

## 9. 发布意见

软件候选版本已具备可复现构建和回归依据，Debug/Release均有充分容量余量。建议先审查全部diff及未跟踪新增文件，再进行以上板上验收；通过后由用户决定commit、Tag与v1.0.0发布。当前不建议标注“全部硬件/功耗/掉电验收完成”。

## 10. 修改文件清单与原因

| 文件 | 修改原因 |
|---|---|
| `App/Monitor/monitor.c` | C05/C07单位、有效性和旧显示覆盖 |
| `App/Sensor/light_state.c` | C02/C08阈值与Pending失效 |
| `App/Sensor/light_state.h` | C02/C08阈值与Pending失效 |
| `App/Sensor/motion_state.c` | C08和有符号幅值边界 |
| `App/Sensor/motion_state.h` | C08和有符号幅值边界 |
| `App/Sensor/sensor_manager.c` | C08/C09有效性、恢复、校准和数据接口 |
| `App/Sensor/sensor_manager.h` | C08/C09有效性、恢复、校准和数据接口 |
| `App/Sensor/sensor_state.h` | C08/C09有效性、恢复、校准和数据接口 |
| `App/app.c` | 服务推进、启动降级、健康LED和保留WFI调度 |
| `App/data_logger.c` | C03代际、序号、CRC/提交、恢复、分步I/O和失败可观测性 |
| `App/data_logger.h` | C03代际、序号、CRC/提交、恢复、分步I/O和失败可观测性 |
| `CMakeLists.txt` | C04有效目标选项、GC、Os、HEX/map/size及自有代码Werror |
| `Core/Inc/stm32f1xx_it.h` | 同步移除无业务PB5 EXTI，保持输入确定电平 |
| `Core/Src/main.c` | 同步移除无业务PB5 EXTI，保持输入确定电平 |
| `Core/Src/stm32f1xx_it.c` | 同步移除无业务PB5 EXTI，保持输入确定电平 |
| `Core/Src/syscalls.c` | 显式返回/字符转换，消除应用构建告警 |
| `Drivers/Debug/debug.c` | C06格式/整数边界与UART总期限 |
| `Drivers/Debug/debug.h` | C06格式/整数边界与UART总期限 |
| `Drivers/MPU6050/mpu6050.c` | C09符号/偏置饱和、身份/配置及增量校准 |
| `Drivers/MPU6050/mpu6050.h` | C09符号/偏置饱和、身份/配置及增量校准 |
| `Drivers/OLED/oled.c` | ACK、时序、分块刷新、参数边界或字模括号告警（字节不变） |
| `Drivers/OLED/oled.h` | ACK、时序、分块刷新、参数边界或字模括号告警（字节不变） |
| `Drivers/OLED/oled_font.c` | ACK、时序、分块刷新、参数边界或字模括号告警（字节不变） |
| `Drivers/W25Q64/w25q64.c` | C01正确协议、期限、WEL/ID、状态返回及参数检查 |
| `Drivers/W25Q64/w25q64.h` | C01正确协议、期限、WEL/ID、状态返回及参数检查 |
| `Libs/Filter/moving_avg.c` | int64累加避免通用int32输入溢出，明确索引转换 |
| `Libs/Filter/moving_avg.h` | int64累加避免通用int32输入溢出，明确索引转换 |
| `README.md` | C10真实能力、单位、格式迁移、执行证据及硬件限制 |
| `STM32-LowPower-SensorNode.ioc` | 同步移除无业务PB5 EXTI，保持输入确定电平 |
| `cmake/gcc-arm-none-eabi.cmake` | 纯C工具链去除不必要的G++依赖 |
| `docs/closeout-report.md` | C10真实能力、单位、格式迁移、执行证据及硬件限制 |
| `docs/log-format.md` | C10真实能力、单位、格式迁移、执行证据及硬件限制 |
| `docs/notes-mpu6050-zh.md` | C10真实能力、单位、格式迁移、执行证据及硬件限制 |
| `tests/CMakeLists.txt` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/include/main.h` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/include/stm32f1xx_hal.h` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/run_validation.sh` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/test_adc.c` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/test_app.c` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/test_debug.c` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/test_filter.c` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/test_flash.c` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/test_fsm.c` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/test_logger.c` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/test_monitor.c` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/test_oled.c` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/test_sensors.c` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
| `tests/verify_hex.py` | 真实模块回归、HAL/NOR桩或可复现构建/HEX验证 |
