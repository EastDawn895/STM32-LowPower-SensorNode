# STM32-LowPower-SensorNode

A low-power real-time data acquisition and monitoring node built on an STM32F103C8T6 (Blue Pill).

The project is a from-scratch migration from the legacy Keil + Standard Peripheral Library (SPL)
toolchain to a modern **STM32CubeMX + HAL + CMake + GCC** workflow, developed in CLion.

> **Status:** early stage. The HAL project skeleton, debug output, system clock, and a
> non-blocking scheduler are implemented and running on hardware. Sensor acquisition and the
> remaining features are planned (see [Roadmap](#roadmap)).

## Hardware

| Component | Detail |
|-----------|--------|
| MCU       | STM32F103C8T6 (Cortex-M3, 64 KiB Flash, 20 KiB SRAM) |
| Board     | Blue Pill |
| Crystal   | 8 MHz HSE → PLL ×9 → 72 MHz system clock |
| Debug     | USART1 (PA9 TX / PA10 RX) @ 115200 8N1 |
| Indicator | LED on PA6 |

## Current Features

- [x] CubeMX + HAL project skeleton with CMake + GCC (arm-none-eabi)
- [x] 72 MHz system clock (HSE + PLL ×9, Flash latency 2 WS)
- [x] Debug UART driver with decoupled handle injection (`Drivers/Debug`)
- [x] App layer with a **non-blocking 1 s scheduler** (`HAL_GetTick()` + period compare,
      no busy-wait `HAL_Delay`)
- [x] LED heartbeat on PA6

## Planned / In Progress

- [ ] I2C + MPU6050 (accelerometer + gyroscope) data acquisition
- [ ] Sensor data filtering (low-pass / complementary filter)
- [ ] State machine for sleep / sample / alarm scheduling
- [ ] Alarm / threshold detection
- [ ] SPI + W25Q64 flash storage
- [ ] RTOS port (FreeRTOS)
- [ ] Low-power modes (Sleep / Stop / Standby)

## Project Structure

```
STM32-LowPower-SensorNode/
├── App/                     # Application layer (app.c / app.h)
├── Drivers/
│   ├── Debug/               # Debug UART driver
│   ├── CMSIS/               # CMSIS headers (CubeMX)
│   └── STM32F1xx_HAL_Driver/ # STM32 HAL library
├── Core/
│   ├── Inc/                 # main.h, HAL config, IRQ handlers
│   └── Src/                 # main.c, HAL MSP, startup support
├── cmake/                   # Toolchain + CubeMX CMake glue
├── startup_stm32f103xb.s    # Startup file
├── STM32F103xx_FLASH.ld     # Linker script
├── STM32-LowPower-SensorNode.ioc  # CubeMX project
└── CMakeLists.txt
```

The user code lives in `App/` and `Drivers/Debug/` and is kept out of the CubeMX-generated
`Core/` files as much as possible, so re-generating code from the `.ioc` does not clobber it.

## Build

Prerequisites:

- ARM GNU Toolchain (`arm-none-eabi-gcc`)
- CMake ≥ 3.22
- CLion (or any CMake-capable IDE / command line)

The project is generated as a CMake project from the `.ioc` file. Open the folder in CLion and
build, or from the command line:

```bash
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
cmake --build build
```

Flash the resulting `build/STM32-LowPower-SensorNode.elf` to the board with ST-Link (via
OpenOCD / GDB or STM32CubeProgrammer).

## Roadmap

1. **v0.1** — HAL skeleton, debug UART, 72 MHz clock, non-blocking scheduler, LED heartbeat
2. **v0.2** — MPU6050 acquisition over I2C
3. **v0.3** — Filtering + state machine
4. **v1.0** — Alarm, flash storage, low-power modes

## License

TBD
