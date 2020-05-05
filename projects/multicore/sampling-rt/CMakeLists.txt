# This code is based on a sample from Microsoft (see license below),
# with modifications made by MediaTek.
# Modified version of CMakeLists.txt from Microsoft Azure Sphere sample code:
# https://github.com/Azure/azure-sphere-samples/blob/master/Samples/HelloWorld/HelloWorld_RTApp_MT3620_BareMetal/CMakeLists.txt

#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

# Configurations
project(sampling-rt C CXX)
azsphere_configure_tools(TOOLS_REVISION "20.04")
azsphere_configure_api(TARGET_API_SET "5+Beta2004")
add_compile_definitions(OSAI_FREERTOS)
add_compile_definitions(OSAI_ENABLE_DMA)
add_link_options(-specs=nano.specs -specs=nosys.specs)

# Executable
add_executable(${PROJECT_NAME}
               main.cpp
               CaptureTask.cpp
               CalculateTask.cpp
               MboxTask.cpp
               drivers/AnalogIn.cpp
               drivers/DigitalOut.cpp
               drivers/Ticker.cpp
               drivers/MailboxHLCore.cpp
               ../../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_uart.c
               ../../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_dma.c
               ../../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpio.c
               ../../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpt.c
               ../../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_adc.c
               ../../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_mbox.c
               ../../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_mbox_shared_mem.c)

# Include Folders
include_directories(${PROJECT_NAME} PUBLIC
                    drivers/
                    ./)
target_include_directories(${PROJECT_NAME} PUBLIC
                           ../../../mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/inc
                           ./)

# Libraries
set(OSAI_FREERTOS 1)
add_subdirectory(../../../mt3620_m4_software/MT3620_M4_Driver ./lib/MT3620_M4_Driver)
target_link_libraries(${PROJECT_NAME} MT3620_M4_Driver)

# Linker, Image
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)
azsphere_target_add_image_package(${PROJECT_NAME})
