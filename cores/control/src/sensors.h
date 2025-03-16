#ifndef SENSORS_H_
#define SENSORS_H_

#include <zephyr/kernel.h>

enum sensor_reading_type {
#ifdef CONFIG_APP_USE_SHT40
    SENSOR_PKT_SHT,
#endif
#ifdef CONFIG_APP_USE_IMU
    SENSOR_PKT_IMU,
#endif
};

typedef struct sensor_reading {
    int err;
    int type;

#ifdef CONFIG_APP_USE_SHT40
    double hum;
    double temp;
#endif

#ifdef CONFIG_APP_USE_IMU
    double accel_x;
    double accel_y;
    double accel_z;

    double gyro_x;
    double gyro_y;
    double gyro_z;

    double mag_x;
    double mag_y;
    double mag_z;
#endif

} sensor_reading_t;

#ifdef CONFIG_APP_USE_SHT40
void sht4x_poll(void);
#endif

#ifdef CONFIG_APP_USE_IMU
int imu_set_sampling_frequency(double freq);
void imu_poll(void);
#endif

int sensor_reading_get(void *data, k_timeout_t timeout);

#endif
