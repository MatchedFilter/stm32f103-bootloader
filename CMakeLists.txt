cmake_minimum_required(VERSION 3.22)
project(stm32f103-bootloader C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -mcpu=cortex-m3
    -mthumb
    -Wall
    -Wextra
    -fdata-sections
    -ffunction-sections
    -O0 -g3
)

add_link_options(
    -mcpu=cortex-m3
    -mthumb
    -specs=nano.specs
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -T${CMAKE_CURRENT_SOURCE_DIR}/src/STM32F103C8Tx_FLASH.ld
)

add_compile_definitions(USE_HAL_DRIVER STM32F103xB)

include_directories(
    include
    bsp/drivers/STM32F1xx_HAL_Driver/Inc
    bsp/drivers/STM32F1xx_HAL_Driver/Inc/Legacy
    bsp/drivers/CMSIS/Device/ST/STM32F1xx/Include
    bsp/drivers/CMSIS/Include
    bsp/Core/Inc
    bsp/drivers/STM32_USB_Device_Library/Core/Inc
    bsp/drivers/STM32_USB_Device_Library/Class/CDC/Inc
)

file(GLOB_RECURSE SOURCES 
    "src/*.c"
    "src/*.s"
    "bsp/drivers/STM32F1xx_HAL_Driver/Src/*.c"
    "bsp/drivers/STM32_USB_Device_Library/*.c"
    "bsp/Core/Src/*.c"
    "bsp/Core/Src/*.s"
)

set_source_files_properties(bsp/Core/Src/startup_stm32f103xb.S
    PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp"
)

add_executable(${PROJECT_NAME}.elf ${SOURCES})

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
