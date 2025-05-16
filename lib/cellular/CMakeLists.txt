zephyr_library()
zephyr_library_sources(
  cellular.c
  cellular_queue.c
  cellular_packet.c
)

zephyr_library_sources_ifdef(CONFIG_CELLULAR_BACKEND_NRF cellular_backend_nrf.c)
