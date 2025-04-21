# This CMake file is picked by the Zephyr build system because it is defined
# as the module CMake entry point (see zephyr/module.yml).

# When libraries/drivers are written, uncomment the following lines.

zephyr_syscall_include_directories(include)

zephyr_include_directories(include)

# add_subdirectory(drivers)
add_subdirectory(lib)
