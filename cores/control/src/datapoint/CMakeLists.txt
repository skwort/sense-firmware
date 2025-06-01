target_sources(app PRIVATE datapoint.c datapoint_helpers.c datapoint_queue.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
