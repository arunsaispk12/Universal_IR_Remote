# Universal IR Remote - ESP32 RainMaker Project
# CMake configuration

cmake_minimum_required(VERSION 3.16)

# Target is set via 'idf.py set-target <target>' - do not hardcode here

# Read version from version.txt
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/version.txt" PROJECT_VER)
string(STRIP "${PROJECT_VER}" PROJECT_VER)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project name and version
project(universal_ir_remote VERSION ${PROJECT_VER})

# Add compile options
target_compile_options(${CMAKE_PROJECT_NAME}.elf PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)
