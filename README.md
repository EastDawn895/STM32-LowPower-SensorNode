# 🚀 LowPower-SensorNode (Low-Power Multi-Sensor Anomaly Blackbox)

A lightweight, low-power, non-blocking multi-sensor monitoring node based on the STM32F103C8T6. Built entirely in pure C without any RTOS, this project implements a pseudo-Tickless sleep scheduling system via `HAL_GetTick()`, a soft state machine with hysteresis debouncing, and a ring-buffer-based Flash power-loss blackbox data logger.

## 🛠 Tech Stack & Development Environment
* **MCU**: STM32F103C8T6 (ARM Cortex-M3 @ 72MHz)
* **Environment**: Ubuntu 24.04 + CLion + CMake + Ninja
* **Toolchain**: `arm-none-eabi-gcc` (STM32CubeCLT)
* **Core Peripherals**: MPU6050 (I2C2), Photoresistor (ADC1), W25Q64 (SPI2), SSD1306 OLED (Soft-I2C)

---

## 🏗 System Architecture

The system features a strict multi-layer architecture, achieving complete decoupling between hardware drivers, core algorithms, and application scheduling:

```text
+-------------------------------------------------------------------+
|                         App Layer (Scheduling)                    |
|                                                                   |
|   +-------------------+  +------------------+  +----------------+ |
|   |   SensorManager   |  |   DataLogger     |  |    Monitor     | |
|   | (100ms Tick Rate) |  |   (Edge Trigger) |  | (1s UI Refresh)| |
|   +-------------------+  +------------------+  +----------------+ |
+-------------------------------------------------------------------+
|                       Core Logic & Algorithms                     |
|                                                                   |
|   +-------------------+  +------------------+                     |
|   |  Moving Average   |  |   FSM            | <-- Hysteresis &    |
|   |  Filter           |  |  (Motion/Light)  |     Debounce Filter |
|   +-------------------+  +------------------+                     |
+-------------------------------------------------------------------+
|                         Hardware Drivers                          |
|                                                                   |
|   +-------+  +-----------+  +-------------+  +---------------+    |
|   |  ADC  |  | Hardware  |  |  Hardware   |  | Software I2C  |    |
|   |       |  | I2C (DMA) |  | SPI (Polling)|  | (GPIO Bitbang)|    |
|   +-------+  +-----------+  +-------------+  +---------------+    |
|    Light        MPU6050          W25Q64            SSD1306        |
+-------------------------------------------------------------------+
✨ Core Features & Engineering Implementation
1. Non-blocking FSM & Hysteresis
Eliminates blocking HAL_Delay() completely, relying on timestamp management for periodic sensor sampling and multitasking.
A hysteresis comparator algorithm is introduced into the state evaluation, completely resolving false triggers caused by sensor reading fluctuations around a single threshold.

Plaintext
       [ Motion State Transition Logic ]
                  
           +-----------+
      +--> |   IDLE    | --- (Gx > Threshold A) --->+
      |    +-----------+                            |
(Gx < Threshold B)                                  |
      |    +-----------+                            |
      +--- |  ACTIVE   | <--------------------------+
           +-----------+
    * The gap between Threshold A and B creates a hysteresis band, 
      effectively filtering out physical vibration noise.
2. Edge-Triggered Flash Data Logger
To prevent Flash wear-out from continuous anomaly writing, the system logs data strictly at the moment of state transition (edge). Upon power-on/reset, the system automatically scans the Flash memory to locate the next blank address for continuous writing.

Flash Memory Layout & Ring Buffer Mechanism:
The log struct is strictly 32 Bytes aligned. The first 8192 bytes (2 sectors) act as a ring buffer, capable of storing 256 high-precision historical records.

Plaintext
[ 32-Byte Aligned SensorLog_t ]
  0      4      8      9      10             32 (Bytes)
  +------+------+------+------+--------------+
  | Time | Value|Source|LState| Reserved(22) |
  +------+------+------+------+--------------+

[ Flash Ring Address Allocation & Erase Policy ]
  0x0000        0x1000 (4096)   0x2000 (8192)
  |===============|===============| -> % 8192 (Auto wraps back to 0)
     Sector 0         Sector 1
  (128 Entries)    (128 Entries)
  * Automatically triggers W25Q64_EraseSector() when the write cursor 
    crosses the 4096-byte boundary, achieving seamless loop writing.
3. Pseudo-Tickless Sleep Mode Scheduling
After processing the 100ms/1000ms business logic in the main loop (while(1)), the system immediately yields CPU control via HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI).
Relying on the SysTick 1ms interrupt to wake the CPU for brief timestamp checks, the system remains in low-power sleep mode ~99% of the time, resulting in a dramatic reduction in dynamic power consumption.

🚀 Quick Start
Build & Compile (CMake)
Bash
# Generate build files
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=gcc-arm-none-eabi.cmake

# Compile and generate .elf / .hex
cmake --build cmake-build-debug
Serial Output Example (Baud: 115200)
Plaintext
System Init OK
Init s_writeAddr:0
AX=-0.64 Gx=0.00 GxF=0.5 State_M=IDLE   Light=1496 Filter=1425 State_L=HIGH
Logger: Light Triggered!
readBack_Time:17369 Source:2 LState:0 Value:3748 s_waddr:32
AX=-0.64 Gx=434.8 GxF=51.4 State_M=ACTIVE Light=1986 Filter=1984 State_L=NORMAL
Logger: Motion Triggered!
readBack_Time:26861 Source:1 LState:1 Value:2853 s_waddr:64