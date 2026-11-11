cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Unit-TrackBall)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/User/flash/user_flash.c
    Core/User/i2c/user_i2c.c
    Core/User/i2c/user_i2c_callback.c
    Core/User/key/user_key.c
    Core/User/led/user_led.c
    Core/User/uid/user_uid.c
    Core/User/mot6010_official/sensor_driver/src/mixo_ots_access.c
    Core/User/mot6010_official/sensor_driver/src/mixo_ots_driver.c
    Core/User/mot6010_official/port/mixo_gpio_i2c.c
    Core/User/mot6010_official/port/mixo_hal_timer.c
    Core/User/mot6010_official/port/mixo_ots_app.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/User/flash
    Core/User/i2c
    Core/User/key
    Core/User/led
    Core/User/uid
    Core/User/mot6010_official/sensor_driver/include
    Core/User/mot6010_official/mixo_hal/include
    Core/User/mot6010_official/port
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>" "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex"
    COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>" "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin"
    BYPRODUCTS
        "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex"
        "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin"
    COMMENT "Generating ${CMAKE_PROJECT_NAME}.hex and ${CMAKE_PROJECT_NAME}.bin"
)

find_program(STM32_PROGRAMMER_CLI
    NAMES STM32_Programmer_CLI
    HINTS
        "/Applications/STMicroelectronics/STM32Cube/STM32CubeProgrammer/STM32CubeProgrammer.app/Contents/MacOs/bin"
    DOC "Path to STM32CubeProgrammer command line tool"
)

set(STM32_PROGRAMMER_PORT "JLINK" CACHE STRING "STM32CubeProgrammer connection port, for example JLINK or SWD")
set(STM32_PROGRAMMER_MODE "NORMAL" CACHE STRING "STM32CubeProgrammer connection mode")
set(STM32_PROGRAMMER_RESET_MODE "SWrst" CACHE STRING "STM32CubeProgrammer reset mode")
set(STM32_PROGRAMMER_SN "" CACHE STRING "Optional debug probe serial number")
set(STM32_PROGRAMMER_RESET_AFTER_FLASH OFF CACHE BOOL "Reset the MCU after flashing")

set(STM32_PROGRAMMER_CONNECT_ARGS
    -c
    "port=${STM32_PROGRAMMER_PORT}"
    "mode=${STM32_PROGRAMMER_MODE}"
    "reset=${STM32_PROGRAMMER_RESET_MODE}"
)

if(STM32_PROGRAMMER_SN)
    list(APPEND STM32_PROGRAMMER_CONNECT_ARGS "sn=${STM32_PROGRAMMER_SN}")
endif()

set(STM32_PROGRAMMER_FLASH_ARGS
    -w "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
    -v
)

if(STM32_PROGRAMMER_RESET_AFTER_FLASH)
    list(APPEND STM32_PROGRAMMER_FLASH_ARGS -rst)
endif()

if(STM32_PROGRAMMER_CLI)
    add_custom_target(flash
        COMMAND "${STM32_PROGRAMMER_CLI}" ${STM32_PROGRAMMER_CONNECT_ARGS} ${STM32_PROGRAMMER_FLASH_ARGS}
        DEPENDS ${CMAKE_PROJECT_NAME}
        USES_TERMINAL
        COMMENT "Flashing ${CMAKE_PROJECT_NAME} with STM32CubeProgrammer"
    )
else()
    add_custom_target(flash
        COMMAND ${CMAKE_COMMAND} -E echo "STM32_Programmer_CLI not found. Install STM32CubeProgrammer or configure with -DSTM32_PROGRAMMER_CLI=/path/to/STM32_Programmer_CLI"
        COMMAND ${CMAKE_COMMAND} -E false
    )
endif()
