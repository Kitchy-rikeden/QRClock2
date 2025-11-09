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
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# include WIFI_SSID & WIFI_PASSWORD
# secrets.cmake example:
# | set(WIFI_SSID "hoge")
# | set(WIFI_PASSWORD "fuga")
include(secrets.cmake)

project(QRClock2 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(QRClock2 
    QRClock2.c
    tm1640.c
    display.c
    rotary.c
    ds1302.c
    ntp_client.c
    analog.c
)

# https://github.com/fukuchi/libqrencode
set(PROJECT_VERSION_MAJOR 4)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 1)
add_definitions(-DMAJOR_VERSION=${PROJECT_VERSION_MAJOR})
add_definitions(-DMINOR_VERSION=${PROJECT_VERSION_MINOR})
add_definitions(-DMICRO_VERSION=${PROJECT_VERSION_PATCH})
add_definitions(-DVERSION="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
add_definitions(-DHAVE_SDL=0)
add_compile_definitions(STATIC_IN_RELEASE=static)
add_library(qrencode
    libqrencode/bitstream.c
    libqrencode/mask.c
    libqrencode/mmask.c
    libqrencode/mqrspec.c
    libqrencode/qrenc.c
    libqrencode/qrencode.c
    libqrencode/qrinput.c
    libqrencode/qrspec.c
    libqrencode/rsecc.c
    libqrencode/split.c
)

pico_set_program_name(QRClock2 "QRClock2")
pico_set_program_version(QRClock2 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(QRClock2 0)
pico_enable_stdio_usb(QRClock2 1)

# Add the standard library to the build
target_link_libraries(QRClock2
        pico_cyw43_arch_lwip_poll
        pico_stdlib
        qrencode
)

# Add the standard include files to the build
target_include_directories(QRClock2 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
)

target_include_directories(qrencode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libqrencode
)

target_compile_definitions(QRClock2 PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
)

pico_add_extra_outputs(QRClock2)

