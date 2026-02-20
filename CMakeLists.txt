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
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Projet C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# --- 1. LIB LoRa et Test ---
add_subdirectory(common/LoRa)
add_subdirectory(tests/LoRa)
add_subdirectory(tests/DialogLoRa)


# --- 2. DÉFINITION DE L'EXÉCUTABLE  ---
add_executable(Projet main.cpp)
target_link_libraries(Projet PRIVATE lora_common pico_stdlib)

# --- 3. OPTIONS PICO  ---
pico_set_program_name(Projet "Projet")
pico_set_program_version(Projet "0.1")

# Configuration de la console (USB activé, UART désactivé)
pico_enable_stdio_uart(Projet 0)
pico_enable_stdio_usb(Projet 1)

# Cette ligne est aussi cruciale pour générer le .uf2 final
pico_add_extra_outputs(Projet)
