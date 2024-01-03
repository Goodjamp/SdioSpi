cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")

project(SdSpi C ASM)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set( APP_SRC
    App/main.c
    App/DebugServices/DebugServices.c
    App/DebugServices/DebugServices.h
    App/RingBuff/RingBuff.c
    App/RingBuff/RingBuff.h
    App/SdSpiExample/SdSpiExample.c
    App/SdSpiExample/SdSpiExample.h
)

set( APP_PATH
    App
    App/DebugServices
    App/RingBuff
    App/SdSpiExample
)

set( BSP_SRC
    BSP/Spi/Spi.c
    BSP/Services/Services.c
    BSP/SystemClock/SystemClock.c
)

set( BSP_PATH
    BSP
    BSP/Spi
    BSP/Services
    BSP/SystemClock
)

set( GENERYC_SRC
    MCU/Src/stm32f4xx_it.c
   # MCU/Src/system_stm32f4xx.c
    MCU/startup_stm32f411ceux.s
    MCU/CMSIS/Device/ST/STM32F4xx/Source/system_stm32f4xx.c

    HAL/STM32F4xxll_Driver/Src/stm32f4xx_ll_gpio.c
    HAL/STM32F4xxll_Driver/Src/stm32f4xx_ll_dma.c
    HAL/STM32F4xxll_Driver/Src/stm32f4xx_ll_rcc.c
    HAL/STM32F4xxll_Driver/Src/stm32f4xx_ll_utils.c
    HAL/STM32F4xxll_Driver/Src/stm32f4xx_ll_exti.c
    HAL/STM32F4xxll_Driver/Src/stm32f4xx_ll_i2c.c
    HAL/STM32F4xxll_Driver/Src/stm32f4xx_ll_spi.c
    HAL/STM32F4xxll_Driver/Src/stm32f4xx_ll_tim.c
    HAL/STM32F4xxll_Driver/Src/stm32f4xx_ll_usart.c

    Lib/SdSpi/SdSpi.c
    Lib/SdSpi/SdSpi.h
    Lib/SdSpi/SdSpiInternal.h
)

set(GENERYC_PATH
    MCU/
    MCU/CMSIS/Device/ST/STM32F4xx/Include
    MCU/CMSIS/Include
    MCU/Inc
    MCU/Src

    HAL/STM32F4xxll_Driver/Src
    HAL/STM32F4xxll_Driver/Inc

    Lib/SdSpi
)

set (RTT_SRC
    Middlewares/Third_Party/RTT/Syscalls/SEGGER_RTT_Syscalls_GCC.c
    Middlewares/Third_Party/RTT/RTT/SEGGER_RTT.c
    Middlewares/Third_Party/RTT/RTT/SEGGER_RTT_printf.c
    Middlewares/Third_Party/RTT/RTT/SEGGER_RTT_ASM_ARMv7M.S
)

set (RTT_PATH
    Middlewares/Third_Party/RTT/Config
    Middlewares/Third_Party/RTT/RTT
    Middlewares/Third_Party/RTT/Syscalls
)

set (FAT_SRC
    Middlewares/FAT/source/diskio.c
    Middlewares/FAT/source/diskio.h
    Middlewares/FAT/source/ff.c
    Middlewares/FAT/source/ff.h
    Middlewares/FAT/source/ffconf.h
    Middlewares/FAT/source/ffsystem.c
    Middlewares/FAT/source/ffunicode.c
)

set (FAT_PATH
    Middlewares/FAT/source
)

add_definitions(
    -DUSE_FULL_LL_DRIVER
    -DSTM32F411xE

    -DCSFR_MAX_PACKET_SIZE=${CSFR_MAX_PACKET_SIZE}
)
include_directories(${APP_PATH} ${BSP_PATH} ${GENERYC_PATH} ${RTT_PATH} ${FAT_PATH})

set(COMPILER_GENERAL_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-common")
set(COMPILER_USER_FLAGS    "-g3 -Wall -Wno-multichar -fdata-sections -ffunction-sections -fno-strict-aliasing -ffreestanding -fno-builtin -fno-common")

set(LINKER_SCRIPT MCU/STM32F411CEUx_FLASH.ld)
set(LINKER_USER_FLAGS
    -T${LINKER_SCRIPT}
    -L${CMAKE_CURRENT_SOURCE_DIR}
    --specs=nosys.specs
    --specs=nano.specs
    -fdata-sections
    -ffunction-sections
    -lc
    -lm
    -lnosys

    -mcpu=cortex-m4
    -mfloat-abi=hard
)

set (CMAKE_EXE_LINKER_FLAGS "-Xlinker -print-memory-usage -Xlinker -Map=output.map -Xlinker --gc-sections -Xlinker --sort-section=alignment -Xlinker --cref")
add_link_options(${LINKER_USER_FLAGS})

set(TargetMcuImageElf ${CMAKE_PROJECT_NAME}.elf)

set(CMAKE_CXX_FLAGS "${COMPILER_GENERAL_FLAGS} ${COMPILER_USER_FLAGS}")
if ("${BUILD_MODE}" STREQUAL "RELEASE")
    set(CMAKE_C_FLAGS "${COMPILER_GENERAL_FLAGS} ${COMPILER_USER_FLAGS} -std=gnu99 -O3")
elseif ("${BUILD_MODE}" STREQUAL "DEBUG")
    set(CMAKE_C_FLAGS "${COMPILER_GENERAL_FLAGS} ${COMPILER_USER_FLAGS} -O0")
else ()
    set(BUILD_MODE "DEBUG")
    set(CMAKE_C_FLAGS "${COMPILER_GENERAL_FLAGS} ${COMPILER_USER_FLAGS} -O0")
endif()

message(STATUS, "CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS, "C_FLAGS: ${CMAKE_C_FLAGS}")

# Add target #1 ${CMAKE_PROJECT_NAME}.elf = RcNetwork.elf
add_executable(${TargetMcuImageElf} ${APP_SRC} ${BSP_SRC} ${GENERYC_SRC} ${RTT_SRC} ${FAT_SRC})

set(McuImageHex ${CMAKE_PROJECT_NAME}.hex)

# Notification about current build mode
add_custom_command (
    TARGET ${TargetMcuImageElf} PRE_BUILD
    COMMENT "Build mode: ${BUILD_MODE}"
)

# Create iHex fw
add_custom_command (
    TARGET ${TargetMcuImageElf} POST_BUILD
    COMMENT "Generate hex ${McuImageHex}"
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${TargetMcuImageElf} ${McuImageHex}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
