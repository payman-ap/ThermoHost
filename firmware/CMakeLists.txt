# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD waveshare_rp2040_zero CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(firmware C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(firmware firmware.c )

pico_set_program_name(firmware "firmware")
pico_set_program_version(firmware "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(firmware 0)
pico_enable_stdio_usb(firmware 1)

# Add the standard library to the build
target_link_libraries(firmware
    hardware_spi
    pico_stdlib)

# Add the standard include files to the build
target_include_directories(firmware PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(firmware)

# Separate target for testing USB enumeration and WinUSB binding.
add_executable(usb_probe
    usb_probe.c
    usb/usb_descriptors.c
    usb/winusb_descriptors.c
)

# Use our TinyUSB configuration for this target.
target_include_directories(usb_probe PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/usb
)

# Link the USB device stack and flash-based serial number support.
target_link_libraries(usb_probe
    pico_stdlib
    pico_unique_id
    tinyusb_device
)

# The custom descriptors own USB; disable SDK console interfaces.
pico_enable_stdio_usb(usb_probe 0)
pico_enable_stdio_uart(usb_probe 0)

pico_set_program_name(usb_probe "ThermoHost USB probe")
pico_set_program_version(usb_probe "0.1")

pico_add_extra_outputs(usb_probe)
