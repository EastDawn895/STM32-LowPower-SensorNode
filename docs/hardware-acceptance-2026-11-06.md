# 2026-11-06 实板验收记录

日期/时间：2026-11-06，约12:03起（Asia/Shanghai）。分支：closeout/v1.0.1-candidate。候选固件源码为ad2e143，软件测试提交25d103d；本次开始HEAD为4aa33e3。生产源码未改变。

结论：**READY TO FREEZE**。代理实测已解决LED/旧日志问题并验证基础功能、持久化和轮转；随后用户明确确认剩余三项实物验收完成且无异常，授权合并main并发布v1.0.1。用户确认与代理实测证据分别记录；复位测试仍不替代随机断电。

## 1. 连接、固件与备份

- 实际设备：ST-Link/V2（0483:3748）、CH340（1a86:7523），串口/dev/ttyUSB0，115200/8N1。
- OpenOCD 0.12.0识别Cortex-M3、64 KiB Flash，目标电压约3.25 V；外部Flash实际读得JEDEC EF4017。
- 从0x08000000读取完整64 KiB内部Flash；其加载区与现有Debug ELF生成的22936字节bin逐字节相同，确认实板运行的是候选固件。
- 外部Flash前8192字节两次只读备份一致，SHA256为 `63133b057a71e1b3617714430de7853ae3e53e21a98ce8504b0473b6bac7ce58`。
- 二进制备份持久保存于 `/home/ed/backups/STM32-LowPower-SensorNode/2026-11-06/`，包含迁移前双份、擦除后、复位前后、轮转后及最终快照。哈希及大小见[清单](hardware-evidence-2026-11-06/backup-manifest.txt)。二进制未加入Git，原始文件没有覆盖或删除。

## 2. LED快闪根因与安全处置

初始串口持续显示M=IDLE、L=NORMAL、Log=2、Drop=3、OLED=0。Log=2属于LOGGER_FORMAT_ERROR，优先于业务状态，预期100 ms翻转LED。

Flash备份中，0x0000和0x1000的扇区头均为FF，没有新格式有效头，其他槽非空，并呈旧版32字节结构布局（时间/值/source/state，后部零填充）。例如0x20处为时间11760、值3727、source=2、state=0。新格式恢复代码拒绝无有效头的非空区域，故进入Log=2。**这是旧日志介质与候选格式不兼容，并非LED告警恢复锁存。** 未凭Log=2认定芯片损坏。

向用户明确说明范围及破坏性后，用户回复：“允许，备份后初始化这8 KiB并继续测试”。随后仅向0x0000和0x1000发送4 KiB扇区擦除，检查WEL与BUSY，读取全部8192字节确认均为FF；没有整片擦除，也未触及外部Flash其余区域。复位启动Flash=0/Logger=0，MPU增量校准后有效，Drop=0。

此后用户确认：“OLED恢复，NORMAL时LED慢闪”。没有清零运行中的Drop来隐藏错误；Drop=0是初始化复位后新会话的真实结果。

## 3. 实际执行结果

| 检查 | 实际结果 |
|---|---|
| MPU静止 | 校准期间INVALID/cal=2，完成后AX约-0.02g、GX/GXF接近0，状态IDLE；无长期精度或温漂结论 |
| 真实光照 | 用户操作光敏，串口实际记录HIGH→NORMAL，另记录NORMAL→LOW→NORMAL；均Log=0/Drop=0/OLED=0 |
| 真实运动 | 用户绕X轴转动，实际串口出现ACTIVE后恢复IDLE，包含GXF=-66.22、54.19、-29.12、41.51dps；未用改内存替代真实传感器输入 |
| 正常LED | 调试断点记录tick157146、158146、159146，Light=NORMAL、Motion=IDLE，PA6输出寄存器交替，间隔1000 ms |
| 告警LED | HIGH下20次观测间隔250 ms；LOW与ACTIVE下约246–257 ms，PA6交替，符合250 ms调度目标。断点观测会扰动运行，非示波器测量 |
| OLED | UART最近驱动状态为HAL_OK；用户确认屏幕恢复刷新。未做SCL/SDA上升时间/逻辑分析仪验收 |
| Sleep/Tick | 多次连接时PC位于HAL_PWR_EnterSLEEPMode/WFI，串口、采样与Tick持续推进。支持Sleep运行和唤醒，不证明功耗收益或最大调度延迟 |
| 真实日志 | 读回5条真实事件：seq0/1 HIGH、seq2 LOW、seq3 HIGH、seq4 ACTIVE，CRC、提交标记及所属代际有效；运动记录值为-7848原始滤波计数 |
| 复位恢复/续写 | 软件复位后seq=5、cursor=192、Log=0、Drop=0；通过真实DataLogger_Enqueue注入明确测试标记value=12345，HAL_OK。主循环实际写入seq5，读回验证，前5条原样保留 |
| 实物容量轮转 | 通过临时SRAM测试函数调用真实Logger/SPI，顺序提交260条合成负载（value=100000+i），全部返回成功。结束seq=266、generation=2、current=0、cursor=416、count=0、Drop=0 |
| 轮转读回 | 实际Flash两头代际为2/1；共139条有效记录，序号127–265连续，值与测试负载逐条对应，CRC和提交均正确。保留139而非254符合“轮转整扇区淘汰”的设计 |
| 轮转后复位 | 恢复后又接收真实LOW边沿；观察seq=267/cursor=448/Log=0。最终读回141条连续记录seq127–267、双头仍为2/1，所有非空记录CRC/提交有效 |
| Release短测 | 实际烧录Release并verify成功，启动Flash=0/Logger=0，MPU校准恢复，串口OLED=0、Drop=0，保留已有外部日志；仅短测，未做全部实物输入回归 |
| 最终恢复 | 重新烧录Debug并verify，最终读取完整64 KiB与测试前备份逐字节相同；外部Flash保留本次测试日志。最终CPU已resume并退出OpenOCD |

本次轮转负载是测试注入事件，不冒充260次真实运动。临时124字节程序位于SRAM 0x20001000，注入前备份该区域、完成后恢复；没有写入固件源码或内部Flash。程序使用本次已核对固件的固定函数/变量地址，**不是可在其他版本直接运行的通用测试脚本**。执行期间只服务日志器，不把这段数据用于业务采样连续性或调度延迟验收。

新格式的日志区现已包含本次合成测试记录，原先旧日志只保留在备份中；没有在测试后再次擦空来掩盖记录。

## 4. 调试干扰与未计入结果的尝试

- 首次LED断点采集结束，GDB detach未自动恢复CPU。用户观察到“OLED旧内容不更新、PA6不亮”，与当时冻结在Toggle入口的PC/Tick吻合。显式resume后串口恢复，用户确认OLED及正常慢闪恢复。这是本次调试操作造成的暂停，已纠正，不能算生产代码失效或连续运行证据。
- 后续所有暂停读回均显式resume；Flash快照约需十余秒，期间屏幕/采样暂停，已告知用户。
- 尝试不暂停读取SWD内存时出现间歇全零结果；该CSV未用于LED时序或状态结论，以明确暂停的断点观测、UART和用户观察为证据。
- Release ELF未带可用C类型调试信息，GDB类型变量检查报错，未将该检查写作PASS。Release结果依赖烧录verify和真实串口；随即恢复Debug。
- 初次串口日志打印脚本末尾误用了文本read参数，打印失败但二进制串口日志已保存；后续读取文件确认内容，无数据伪造。

## 5. 可追溯证据

原始串口、断点输出、读回解码、擦除日志、编程verify摘录及备份哈希保存在 [hardware-evidence-2026-11-06](hardware-evidence-2026-11-06/)。文件以.txt保存，避免将构建产物加入Git。证据中的脚本仅作本次操作记录，不应直接重跑（擦除脚本具有破坏性且只获本次授权）。完整临时工具/日志保留在gitignored `build/hardware-closeout/`；二进制快照另有上述持久备份。

- [初始串口](hardware-evidence-2026-11-06/serial-initial.log.txt)
- [真实输入及首次复位](hardware-evidence-2026-11-06/serial-final-actions.log.txt)
- [正常/运动LED](hardware-evidence-2026-11-06/led-healthy.log.txt)
- [实际轮转返回](hardware-evidence-2026-11-06/rollover.log.txt)
- [轮转后读回](hardware-evidence-2026-11-06/records-rollover.json.txt)
- [最终Flash读回](hardware-evidence-2026-11-06/records-final.json.txt)
- [Debug/Release烧录verify摘录](hardware-evidence-2026-11-06/program-verify-excerpt.txt)

## 6. 用户补充验收与冻结决定

软件Debug/Release及三配置主机11/11结果沿用本日已执行证据。最终发布只更新文档，生产代码和测试实现未改变。

2026-11-06，用户在本次聊天明确回复：**“三项已完成且无异常，合并并发布v1.0.1”**。对应询问为外设断线恢复、Flash写入中随机断电、至少30分钟连续运行是否均已实际完成且无异常。

| 项目 | 最终记录 | 证据来源/限制 |
|---|---|---|
| 外设断线恢复 | 用户实测确认通过 | 未附各外设操作步骤和原始日志；不推断已遍历全部故障 |
| Flash写入中随机断电 | 用户实测确认通过 | 未附次数和各提交阶段覆盖；不宣称穷尽全部掉电窗口 |
| 至少30分钟连续运行 | 用户实测确认通过 | 未附全程串口时间序列或最大采样间隔测量 |

前三节的实测记录和第五节原始证据保持不变。此前的“待执行”是当时状态；现在依据用户明确实测确认关闭这三项验收，不伪造代理执行过程或新的仪器记录。原始verification-summary.txt仍保留当时代理未执行这些测试的事实。

按项目验收口径，无已知阻断缺陷，允许合并main并新建v1.0.1，已有v1.0不变。功耗、栈峰值、最大调度延迟和总线波形仍未测量，不宣称具体指标；用户验收缺少原始附件是保留的证据限制。项目在当前功能范围冻结，不再扩大功能或架构。
