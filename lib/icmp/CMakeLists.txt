zephyr_library()
zephyr_library_sources(
  icmp_frame.c
  icmp_queue.c
  icmp.c
)

zephyr_library_sources_ifdef(CONFIG_ICMP_PHY_UART icmp_phy_uart.c)

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)
