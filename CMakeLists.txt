cmake_minimum_required(VERSION 3.14)
project(cmodbus C CXX ASM)
set(CMAKE_C_STANDARD 11)

# see setting
#-DTOOLCHAIN_DIR=/opt/toolchain/gcc-arm-none-eabi-8-2019q3
#-DCMAKE_TOOLCHAIN_FILE=/opt/toolchain/cmake/cortex-m3.cmake

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32F103C8Tx_FLASH.ld)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS_INIT} ${LINKER_SCRIPT}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_INIT} -Og -g3 -DDEBUG")
    add_definitions(-DDEBUG_MSG)
else ()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_INIT} -O3")
endif ()

# 增加宏配置
add_definitions(-DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER)

# 相关头文件
include_directories(common)
include_directories(dirver)
include_directories(module)
include_directories(user)
include_directories(lib/CMSIS/Include)
include_directories(lib/STM32F10x_StdPeriph_Driver/inc)
include_directories(lib/STM32F10x_StdPeriph_Driver/Include)
include_directories(lib/modbus/include)
include_directories(lib/modbus/include/ascii)
include_directories(lib/modbus/include/rtu)
include_directories(lib/modbus/include/tcp)
include_directories(lib/modbus/port)
include_directories(lib/modbus/util)
include_directories(lib/modbus/util/dram)
include_directories(proj/RTE/_Target_1)
include_directories(proj/RTE/Device/STM32F103C8)

# 文件源
file(GLOB_RECURSE SOURCES
        lib/STM32F10x_StdPeriph_Driver/Include/*.h
        lib/STM32F10x_StdPeriph_Driver/inc/*.h
        lib/STM32F10x_StdPeriph_Driver/src/*.c
        lib/CMSIS/Include/cmsis_armcc.h
        lib/CMSIS/Include/cmsis_compiler.h
        lib/CMSIS/Include/core_cm3.h
        lib/modbus/include/*.*
        lib/modbus/src/*.*
        lib/modbus/port/*.*
        lib/modbus/util/dram/heap_4.c
        lib/modbus/util/heap_def.h
        lib/modbus/util/heap.h
        lib/modbus/util/mem.c
        lib/modbus/util/mem.h
        lib/modbus/util/msglink.c
        lib/modbus/util/msglink.h

        common/*.*
        dirver/*.*
        module/*.*
        user/*.*
        startup/startup_stm32f103xb.s
        proj/RTE/_Target_1/RTE_Components.h
        proj/RTE/Device/STM32F103C8/RTE_Device.h
        proj/RTE/Device/STM32F103C8/stm32f10x_conf.h
        proj/RTE/Device/STM32F103C8/system_stm32f10x.c
        )


# build execute file
add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map")

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")

