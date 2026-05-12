# THIS FILE IS AUTO GENERATED FROM THE TEMPLATE! DO NOT CHANGE!

cmake_minimum_required(VERSION 3.22)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)

# Cross-compilers and tools
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(STM32F411BLACKPILL_PLC C CXX ASM)

add_subdirectory(friendly_plc_core)

target_compile_options(friendly_plc_core PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mthumb-interwork
        -mfloat-abi=hard
        -mfpu=fpv4-sp-d16
)

target_compile_definitions(friendly_plc_core PRIVATE
        STM32F411xE
        USE_HAL_DRIVER
        ARM_MATH_CM4
        ARM_MATH_MATRIX_CHECK
        ARM_MATH_ROUNDING
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MCU
set(MCU_FLAGS
        -mcpu=cortex-m4
        -mthumb
        -mthumb-interwork
        -mfloat-abi=hard
        -mfpu=fpv4-sp-d16
)

# Defines
add_compile_definitions(
        DEBUG
        USE_HAL_DRIVER
        STM32F411xE
        ARM_MATH_CM4
        ARM_MATH_MATRIX_CHECK
        ARM_MATH_ROUNDING
)

# Include paths
include_directories(
        Core/Inc
        Drivers/STM32F4xx_HAL_Driver/Inc
        Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
        Middlewares/Third_Party/FreeRTOS/Source/include
        Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
        Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
        Drivers/CMSIS/Device/ST/STM32F4xx/Include
        Drivers/CMSIS/Include
)

# Sources
file(GLOB_RECURSE SOURCES
        Core/*.c
        Core/*.s
        Drivers/*.c
        Middlewares/*.c
)

# Linker script
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32F411CEUX_FLASH.ld )

# Optimization
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    set(OPT_FLAGS -Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    set(OPT_FLAGS -Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    set(OPT_FLAGS -Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    set(OPT_FLAGS -Og -g)
endif ()

add_compile_options(
        ${MCU_FLAGS}
        ${OPT_FLAGS}
        -ffunction-sections
        -fdata-sections
        -fno-common
        -fmessage-length=0
        $<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>
)

add_link_options(
        ${MCU_FLAGS}
        --specs=nano.specs
        --specs=nosys.specs
        -Wl,-gc-sections
        -Wl,--print-memory-usage
        -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map
        -T${LINKER_SCRIPT}
)

add_executable(${PROJECT_NAME}.elf
        ${SOURCES}
        ${LINKER_SCRIPT}
)

target_link_libraries(${PROJECT_NAME}.elf
        PRIVATE
        friendly_plc_core
)

target_include_directories(${PROJECT_NAME}.elf
        PRIVATE
        friendly_plc_core/include
        friendly_plc_core/ports/stm32
)

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMAND ${SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
        COMMENT "Building HEX and BIN files"
)