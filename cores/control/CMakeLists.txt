cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(cores-control LANGUAGES C)

FILE(GLOB app_sources src/*.c)

target_sources(app PRIVATE ${app_sources})

add_subdirectory_ifdef(CONFIG_SENSE_CORE_SENSORS src/sensors)

add_subdirectory(src/datapoint)
add_subdirectory(src/sd_card)
add_subdirectory(src/remote_commands)
