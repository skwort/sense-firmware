zephyr_library()

set(SCHEMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/schemas")
set(DATAPOINT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/datapoint")

if(CONFIG_CZD_DATAPOINT)

  # Conditionally generate the CZD DATAPOINT ZCBOR code if the files don't exist
  # or the schema has been changed.
  if(NOT EXISTS ${DATAPOINT_DIR}/czd_datapoint_types.h OR
     ${SCHEMA_DIR}/datapoint.cddl IS_NEWER_THAN ${DATAPOINT_DIR}/czd_datapoint_types.h)

    set(czd_datapoint_cmd
      zcbor code
        --decode --encode
        --short-names
        -c ${SCHEMA_DIR}/datapoint.cddl
        -t datapoint
        --oc ${DATAPOINT_DIR}/czd_datapoint.c
        --oh ${DATAPOINT_DIR}/czd_datapoint.h
    )

    execute_process(
      COMMAND ${czd_datapoint_cmd}
      COMMAND_ERROR_IS_FATAL ANY
    )

  endif()

  FILE(GLOB datapoint_sources ${DATAPOINT_DIR}/*.c)
  zephyr_library_sources(${datapoint_sources})
  zephyr_library_include_directories(${DATAPOINT_DIR})

endif()

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
