cmake_minimum_required(VERSION 3.13.1)

list(APPEND DTS_ROOT ${CMAKE_SOURCE_DIR}/../../)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(cores-control LANGUAGES C)

# Include libraries
target_include_directories(app PRIVATE ../../include)
add_subdirectory(../../lib/ ${CMAKE_BINARY_DIR}/lib)

FILE(GLOB app_sources src/*.c)

target_sources(app PRIVATE ${app_sources})
