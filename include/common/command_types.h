#ifndef COMMAND_TYPES_H_
#define COMMAND_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

enum command_type {
    CMD_NONE_AVAILABLE = 0,
    CMD_SET_POLL_RATE,
    CMD_SET_RAIL_STATE,
    CMD_TYPE_UNKNOWN
};

enum command_sensors {
    CMD_SENSOR_ADCS = 0,
    CMD_SENSOR_LIS3MDL,
    CMD_SENSOR_LSM6DSO,
    CMD_SENSOR_SHT4X,
    CMD_SENSOR_UNKNOWN
};

enum command_rails {
    CMD_RAIL_5VH = 0,
    CMD_RAIL_UNKNOWN
};


static inline const char *command_sensor_to_str(enum command_sensors sensor)
{
    switch (sensor) {
    case CMD_SENSOR_ADCS:    return "ADCS";
    case CMD_SENSOR_LIS3MDL: return "LIS3MDL";
    case CMD_SENSOR_LSM6DSO: return "LSM6DSO";
    case CMD_SENSOR_SHT4X:   return "SHT4X";
    default:                 return "UNKNOWN_SENSOR";
    }
}

static inline const char *command_rail_to_str(enum command_rails rail)
{
    switch (rail) {
    case CMD_RAIL_5VH: return "5VH";
    default:           return "UNKNOWN_RAIL";
    }
}

static inline bool cmd_is_valid_type(enum command_type val) {
    return val >= 0 && val < CMD_TYPE_UNKNOWN;
}

static inline bool cmd_is_valid_sensor(enum command_sensors val) {
    return val >= 0 && val < CMD_SENSOR_UNKNOWN;
}

static inline bool cmd_is_valid_rail(enum command_rails val) {
    return val >= 0 && val < CMD_RAIL_UNKNOWN;
}

#endif /* COMMAND_TYPES_H_ */
