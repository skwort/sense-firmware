zephyr_library()

if(CONFIG_BOARD_SENSE_CORE_NRF9161_NS OR CONFIG_BOARD_SENSE_CORE_NRF9161)
  zephyr_library_sources(board.c)
endif()
