cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi-gcc.cmake)

project(UAM C ASM)

execute_process(
    COMMAND ${CMAKE_C_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT GCC_VERSION VERSION_EQUAL "15.2.1")
    message(WARNING 
        "Expected Arm GNU Toolchain 15.2.1, got GCC ${GCC_VERSION}. "
        "Build may succeed but binary size may differ."
    )
endif()

# ── MCU Flags ────────────────────────────────────────────────────────────────
set(MCU_FLAGS
    -mcpu=cortex-m7
    -mthumb
    -mfpu=fpv5-d16
    -mfloat-abi=hard
)

# ── Compiler Flags ───────────────────────────────────────────────────────────
add_compile_options(
    ${MCU_FLAGS}
    -std=gnu11
    --specs=nano.specs
    -Og
    -g3
    -Wall
    -Wextra
    -Wswitch-default
    -fdata-sections
    -ffunction-sections
)

# ── Compile Definitions ──────────────────────────────────────────────────────
add_compile_definitions(
    DEBUG
    ARM_MATH_CM7
    USE_PWR_LDO_SUPPLY
    USE_HAL_DRIVER
    STM32H723xx
)

# ── Include Paths ─────────────────────────────────────────────────────────────
include_directories(
    # Core / HAL / CMSIS
    Core/Inc
    Drivers/CMSIS/DSP/Include
    Drivers/CMSIS/Include
    Drivers/CMSIS/Device/ST/STM32H7xx/Include
    Drivers/CMSIS/RTOS2/Include
    Drivers/STM32H7xx_HAL_Driver/Inc
    Drivers/STM32H7xx_HAL_Driver/Inc/Legacy

    # FreeRTOS
    Middlewares/Third_Party/FreeRTOS/Source/include
    Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
    Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F

    # TinyUSB
    Middlewares/Third_Party/tinyusb/src
    Middlewares/Third_Party/tinyusb/src/class/cdc
    Middlewares/Third_Party/tinyusb/src/class/vendor
    Middlewares/Third_Party/tinyusb/src/common
    Middlewares/Third_Party/tinyusb/src/device
    Middlewares/Third_Party/tinyusb/src/osal
    Middlewares/Third_Party/tinyusb/src/portable/synopsys/dwc2

    # Application
    Application/Inc
    Application/Inc/drivers
    Application/Inc/COMM
    Application/Inc/common
    Application/Inc/common/utils
    Application/Inc/MESS
    Application/Inc/SYS
    Application/Inc/CFG
    Application/Inc/DAC
    Application/Inc/MAC
    Application/Inc/afe
    Application/Inc/afe/internal
    Application/Inc/bsp
    Application/Inc/error
    Application/Inc/FILT
    Application/Inc/HIL
    Application/Inc/USB
)

# ── Sources ───────────────────────────────────────────────────────────────────
# ── HAL Sources (exclude template files) ─────────────────────────────────────
file(GLOB HAL_SOURCES
    "Drivers/STM32H7xx_HAL_Driver/Src/*.c"
)
# Remove template files - they are example stubs, not real implementations
list(FILTER HAL_SOURCES EXCLUDE REGEX ".*_template\\.c$")

# ── FreeRTOS Sources ──────────────────────────────────────────────────────────
file(GLOB FREERTOS_SOURCES
    "Middlewares/Third_Party/FreeRTOS/Source/*.c"
    "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/*.c"
)

file(GLOB FREERTOS_HEAP
    "Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c"
)

# Only compile the correct port
file(GLOB FREERTOS_PORT
    "Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/*.c"
)

# ── USB Sources ───────────────────────────────────────────────────────────────
file(GLOB TUSB_SOURCES
    "Middlewares/Third_Party/tinyusb/src/tusb.c"
    "Middlewares/Third_Party/tinyusb/src/class/cdc/*.c"
    "Middlewares/Third_Party/tinyusb/src/class/vendor/*.c"
    "Middlewares/Third_Party/tinyusb/src/common/*.c"
    "Middlewares/Third_Party/tinyusb/src/device/*.c"
    "Middlewares/Third_Party/tinyusb/src/osal/*.c"
    "Middlewares/Third_Party/tinyusb/src/portable/*.c"
)

# ── App / Core ────────────────────────────────────────────────────────────────
file(GLOB_RECURSE APP_SOURCES
    "Application/Src/*.c"
)

file(GLOB_RECURSE CORE_SOURCES
    "Core/Src/*.c"
)

file(GLOB STARTUP_SOURCE
    "Core/Startup/*.s"
)

# ── Combine ───────────────────────────────────────────────────────────────────
set(ALL_SOURCES
    ${APP_SOURCES}
    ${CORE_SOURCES}
    ${STARTUP_SOURCE}
    ${HAL_SOURCES}
    ${FREERTOS_SOURCES}
    ${FREERTOS_HEAP}
    ${FREERTOS_PORT}
    ${TUSB_SOURCES}
)

# ── Linker ───────────────────────────────────────────────────────────────────
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32H723VETX_FLASH.ld)

add_link_options(
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
    --specs=nano.specs
    --specs=nosys.specs
    -static
    -u_printf_float
    -u_scanf_float
    -Wl,--start-group
    -lc
    -lm
    -Wl,--end-group
)

# ── Executable ───────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME}.elf ${ALL_SOURCES})

# CMSIS-DSP prebuilt library
target_link_libraries(${PROJECT_NAME}.elf
    -L${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Lib/GCC
    arm_cortexM7lfsp_math
)

# ── Post-build ────────────────────────────────────────────────────────────────
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE}              $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Generating .hex, .bin and printing size"
)