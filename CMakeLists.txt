cmake_minimum_required(VERSION 3.22)
project(stm32f103-bootloader C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_options(
        -mcpu=cortex-m3
        -mthumb
        -Wall
        -Wextra
        -fdata-sections
        -ffunction-sections
        -Os -g3
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
        "bsp/drivers/STM32F1xx_HAL_Driver/Src/*.c"
        "bsp/drivers/STM32_USB_Device_Library/*.c"
        "bsp/Core/Src/*.c"
        "bsp/Core/Src/*.s"
    )


    set_source_files_properties(bsp/Core/Src/startup_stm32f103xb.S
        PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp"
    )

    add_executable(${PROJECT_NAME}.elf
        ${SOURCES}
        ${COMMON_SRCS}
    )

    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O srec ${PROJECT_NAME}.elf ${PROJECT_NAME}.srec
        COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
        COMMENT "Generating auxiliary flash memory file formats (.bin, .hex, .srec)..."
    )


else()
    message(STATUS "Configuring Target: Native Linux Execution Utility")

    add_compile_options(-Wall -Wextra -O0 -g)
    file(GLOB_RECURSE SOURCES
        "src/*.c"
        "linux-stub/*.c"
    )

    add_executable(${PROJECT_NAME}
        ${SOURCES}
        ${COMMON_SRCS}
    )
    target_include_directories(${PROJECT_NAME} PRIVATE linux-stub)
    

endif()


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
