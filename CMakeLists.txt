# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
set(picoSdk ${USERHOME}/.pico-sdk)
add_definitions(-DPICO_RP2040)
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

#include(pico_sdk_import.cmake)
# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()

include(pico_sdk_import.cmake)

add_subdirectory(pico-cpp)

project(test_modbus C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


pico_sdk_init()

add_executable(test_modbus
    src/main.cpp 
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(test_modbus pico_stdlib hardware_uart)

add_library(FreeRTOS STATIC 
FreeRTOS-Kernel/croutine.c
FreeRTOS-Kernel/event_groups.c
FreeRTOS-Kernel/list.c
FreeRTOS-Kernel/queue.c
FreeRTOS-Kernel/stream_buffer.c
FreeRTOS-Kernel/tasks.c
FreeRTOS-Kernel/timers.c 
FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/port.c
FreeRTOS-Kernel/portable/MemMang/heap_4.c 
src/FreeRTOS_Support_Files/IdleMemory.c)

target_include_directories(FreeRTOS PUBLIC
FreeRTOS-Kernel/include
include/
FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/include
${picoSdk}/sdk/2.1.1/src/common/pico_base_headers/include
${picoSdk}/sdk/2.1.1/src/common/pico_sync/include
${picoSdk}/sdk/2.1.1/src/common/pico_time/include
${picoSdk}/sdk/2.1.1/src/rp2_common/pico_platform_compiler/include
${picoSdk}/sdk/2.1.1/src/rp2_common/pico_platform_sections/include
${picoSdk}/sdk/2.1.1/src/rp2_common/pico_platform_panic/include
${picoSdk}/sdk/2.1.1/src/rp2_common/hardware_sync/include
${picoSdk}/sdk/2.1.1/src/rp2_common/hardware_sync_spin_lock/include
${picoSdk}/sdk/2.1.1/src/rp2_common/hardware_base/include
${picoSdk}/sdk/2.1.1/src/rp2_common/hardware_clocks/include
${picoSdk}/sdk/2.1.1/src/rp2_common/hardware_exception/include
${picoSdk}/sdk/2.1.1/src/rp2_common/hardware_irq/include
${picoSdk}/sdk/2.1.1/src/rp2_common/hardware_timer/include
${picoSdk}/sdk/2.1.1/src/rp2040/pico_platform/include
${picoSdk}/sdk/2.1.1/src/rp2040/hardware_regs/include
${picoSdk}/sdk/2.1.1/src/rp2040/hardware_structs/include
${CMAKE_BINARY_DIR}/generated/pico_base
MODBUS-LIB/Inc)


add_library(Modbus STATIC MODBUS-LIB/Src/Modbus.c)
target_link_libraries(Modbus pico_stdlib hardware_uart)
target_include_directories(Modbus PUBLIC FreeRTOS-Kernel/include include/ FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/include MODBUS-LIB/Inc)


# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(test_modbus)
target_link_libraries(test_modbus hardware_exception FreeRTOS pico-cpp Modbus)
add_custom_command(TARGET test_modbus
        POST_BUILD 
        COMMAND arm-none-eabi-size test_modbus.elf
        )
# rest of your project