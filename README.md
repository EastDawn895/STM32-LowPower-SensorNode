# STM32-LowPower-SensorNode

STM32F103C8T6 裸机多传感器监测与本地事件记录项目。使用 HAL、CMake、Ninja 和 ARM GCC，不使用 RTOS。

**状态：v1.0.1，项目冻结。软件回归及基础实板验收通过；其余三项板测由用户明确确认完成且无异常。各项证据来源与限制见收尾报告。没有远程通信链路。**

## 硬件与当前能力

| 功能 | 接口 / 引脚 | 说明 |
|---|---|---|
| MPU6050 | I2C2 PB10/PB11，100 kHz，AD0接地 | ±2g、±250°/s；运动判断仅使用X轴角速度 |
| 光敏模块 | ADC1 PA0 | 12位原始值，亮暗方向需实测确认 |
| W25Q64 | SPI2 PB13/PB14/PB15，CS PB12 | 9 MHz；支持JEDEC `EF4017`，使用前8 KiB |
| SSD1306 | 软件I2C PB8/PB9，地址0x3C | 开漏、需外部上拉；ACK检查、分块刷新 |
| LED | PA6 | 正常1000 ms、业务告警250 ms、设备/日志错误100 ms翻转；故障优先 |
| UART | USART1 PA9/PA10 | 115200、8N1，本地诊断输出 |
| PB5 | 输入、内部下拉 | 无当前业务，不启用EXTI |

时钟假定HSE为8 MHz，SYSCLK72 MHz、APB1 36 MHz、APB2 72 MHz、ADC12 MHz。不要改变原有电源、总线和引脚接线后仍套用这些假设。

## 软件行为

- App：裸机轮询调度；100 ms采样、1 s诊断/UI内容更新。
- SensorManager：原始/滤波数据分离，八点滑动平均；失败取消Pending计时并清空对应滤波窗口；恢复后重新确认。
- MPU恢复：失败后至少冷却1 s，重新检查身份与配置；等待100 ms后增量采集200个校准样本，样本间隔至少10 ms。**启动和恢复期间保持静止**；范围检查能拒绝部分运动，但不能证明设备静止。有效校准和首次采样完成前显示INVALID。
- MPU内部采样率100 Hz、DLPF配置6；应用读取10 Hz。不是高速冲击检测或全方向运动识别系统。
- UART：AX单位g，GX/GXF单位°/s，两位小数；OLED标明raw的数字仍为原始计数。
- OLED：1 KiB帧缓冲；主循环每次最多发送一个16字节块，异常中止并限频重试。显示不是全屏原子切换，可能短暂出现逐块更新。
- Flash Logger：异常进入边沿入队，固定4条RAM队列；主循环分步执行擦除、校验、编程和提交。`Log`为0表示正常、1为I/O错误、2为格式/恢复歧义；`Drop`是拒收或未确认持久化的事件数。

`OLED=0` 表示 `HAL_OK`。`M=IDLE、L=NORMAL` 只说明业务状态正常；若 `Log=2`，日志被格式保护禁用，LED仍以100 ms翻转（约5 Hz完整亮灭）。异常进入事件会被拒收并增加Drop，稳定状态和回到NORMAL不新增事件。应先备份并检查Flash格式，不能通过清零Drop或取消故障指示来处理。

### 状态阈值

光照按“ADC大对应较暗”的设计解释，需用实际分压电路确认。

| 转换 | 严格条件，连续500 ms |
|---|---|
| NORMAL → LOW | >3200 |
| LOW → NORMAL | <2800 |
| NORMAL → HIGH | <1800 |
| HIGH → NORMAL | >2000 |

等于边界时不启动对应转换；LOW/HIGH不能直接互跳。运动进入为绝对原始滤波值>2000，退出为<1000，确认300 ms。滤波、采样和确认均引入延迟，不宣称瞬时响应。

## 低功耗与阻塞边界

主循环结束执行 `HAL_PWR_EnterSLEEPMode(...WFI)`，SysTick保持1 ms唤醒：这是**普通Sleep，不是Tickless、Stop或Standby**。没有外设断电策略，没有已测功耗或99%休眠指标。

不宣称整个系统“完全非阻塞”：

- UART每次Monitor刷新有20 ms总输出预算；失败或超时后停止本次输出。
- SPI单次命令与负载共享10 ms期限；内部擦除/编程由Logger轮询推进，不在Logger中忙等。兼容同步接口擦除最多等待500 ms、编程10 ms，业务Logger不调用这些同步等待接口。
- OLED每个I2C包5 ms期限，每次Service最多两个包；DWT微秒延时有冻结保护。时序需在Debug/Release下实测。
- MPU传给HAL的通信超时参数为10 ms，但所用HAL还包含固定25 ms的I2C总线BUSY等待；不能将参数当作整个调用的10 ms硬上限。重配置含身份读取和5次写寄存器，期限可累加；ADC转换等待10 ms。
- 启动有Flash10 ms、OLED100 ms稳定延时；Flash启动BUSY等待有500 ms上限。

这些是软件期限，不是整机最坏执行时间测量；采样可能存在抖动。HAL超时依赖SysTick，驱动只允许在中断开启的主循环上下文使用。

## 构建

需要CMake≥3.22、Ninja、ARM GCC及binutils。可使用STM32CubeCLT；工具链搜索PATH、`GCC_TOOLCHAIN_ROOT`（bin目录）及标准 `/opt/st/stm32cubeclt_*` 安装目录。纯C工程无需G++。

在仓库根目录：

```bash
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Release
cmake --build --preset Release
```

生成 `build/Debug` 和 `build/Release` 下的ELF、HEX、map，构建末尾输出size。链接容量保持64 KiB Flash、20 KiB RAM；Debug为Og，Release有效优化为Os，均启用section裁剪。自有及生成应用C文件告警视为错误；厂商HAL告警保留并单独报告，不修改或屏蔽。

首次独立目录构建也可使用正确的工具链路径：

```bash
cmake -S . -B build/readme-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build/readme-debug
```

## 主机测试与可复现验收

只需要本机C编译器，不需要硬件或第三方测试框架。测试编译真实模块源码，HAL由测试桩替代。

```bash
cmake -S tests -B build/host -G Ninja
cmake --build build/host
ctest --test-dir build/host --output-on-failure

cmake -S tests -B build/host-sanitize -G Ninja -DSANITIZE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/host-sanitize
ctest --test-dir build/host-sanitize --output-on-failure
```

Linux GCC/Clang可用时，第二组启用ASan/UBSan。测试在Release下仍保留断言。覆盖滤波、格式、FSM、SPI、日志NOR故障模型、恢复、OLED、Monitor、ADC、无Flash启动降级。

完整验证脚本每次新建构建目录，不删除既有目录，不提交或操作硬件：

```bash
bash tests/run_validation.sh
```

脚本运行Debug/Release独立构建、普通/优化/Sanitizer主机测试、HEX校验及资源统计，输出日志目录。HEX检查也可单独执行：

```bash
python3 tests/verify_hex.py build/Debug/STM32-LowPower-SensorNode.elf build/Debug/STM32-LowPower-SensorNode.hex
```

详见 `docs/closeout-report.md`、`docs/log-format.md` 和 `docs/hardware-acceptance-2026-11-06.md`。

## Flash格式升级与数据安全

**新格式不兼容旧固件日志。检测到未知格式时，默认只读扫描并禁用日志，不自动格式化、不擦除旧数据。** 2026-11-06板测已先双份备份，再经用户明确授权仅初始化前8 KiB，随后Log=0；另短测Release并恢复Debug。完整步骤与证据见板上验收报告。若需启用新格式，先备份并确认旧数据不再需要，再由用户选择外部工具初始化该8 KiB区域；这一步具有破坏性，不由程序自动执行。

新格式每扇区32 B头、127个32 B记录槽，两扇区共254槽。擦除旧扇区时保留条数会减少。记录序号、扇区代际、CRC32和末尾提交标记用于识别完整记录。部分写入的槽不重用；恢复歧义或I/O错误停止写入。I/O错误后需检查硬件并复位恢复日志。

**不是绝对掉电保证**：RAM队列事件可能丢失；擦除目标旧扇区的历史本来就会被淘汰；CRC有碰撞概率；真实Flash掉电行为未验收。Host模型只验证其定义的NOR故障情况。

## 已知限制与验收建议

- 无远程通信、无DMA业务链、无看门狗自动恢复；致命初始化错误和CPU异常保持fail-stop，需调试器或复位排查。
- 已有基础实板证据；外设断线恢复、写入中随机断电及至少30分钟连续运行由用户实测确认通过，未附完整原始记录。总线电气、温漂、最大调度延迟、栈峰值及整板功耗仍无测量结论。
- 软件I2C依赖可用DWT计数器；不可用时有界报错。外部上拉阻值/电容需实测。
- 双扇区方案不提供掉电瞬间RAM事件持久化、无限耐久或始终保留254条历史的承诺。
- v1.0.1按用户验收确认合并main并发布；保留 `closeout/v1.0.1-candidate` 和证据用于追溯，已有v1.0保持不变。项目在现有功能范围冻结。

OLED字模沿用并注明江协科技教程来源；HAL/CMSIS保留其原有许可文件。
