FILE(GLOB heartbeat_sources *.c)
target_sources(app PRIVATE ${heartbeat_sources})
