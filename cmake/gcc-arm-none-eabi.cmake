set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Prefer the toolchain exported by STM32CubeCLT, while still allowing a
# standalone Arm GNU toolchain from PATH. The fallback covers standard
# STM32CubeCLT installations even when the IDE was started before login PATH
# initialization.
file(GLOB _STM32_CUBECLT_GCC_DIRS LIST_DIRECTORIES true
        "/opt/st/stm32cubeclt_*/GNU-tools-for-STM32/bin"
        "/opt/ST/stm32cubeclt_*/GNU-tools-for-STM32/bin"
)
list(SORT _STM32_CUBECLT_GCC_DIRS COMPARE NATURAL ORDER DESCENDING)

find_program(ARM_NONE_EABI_GCC
        NAMES arm-none-eabi-gcc
        HINTS "$ENV{GCC_TOOLCHAIN_ROOT}" ${_STM32_CUBECLT_GCC_DIRS}
        REQUIRED
)
set(CMAKE_C_COMPILER "${ARM_NONE_EABI_GCC}")
set(CMAKE_ASM_COMPILER "${ARM_NONE_EABI_GCC}")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS_INIT
        "-mcpu=cortex-m3 -mthumb"
)

set(CMAKE_CXX_FLAGS_INIT
        "-mcpu=cortex-m3 -mthumb"
)

set(CMAKE_ASM_FLAGS_INIT
        "-mcpu=cortex-m3 -mthumb"
)
