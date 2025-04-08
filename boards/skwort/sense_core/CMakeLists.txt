zephyr_library()

if(BOARD_SENSE_CORE_NRF9161_NS OR BOARD_SENSE_CORE_NRF9161)
  zephyr_library_sources(board.c)
endif()
