#ifndef SENSORS_H_
#define SENSORS_H_

#include <zephyr/kernel.h>

enum sensor_reading_type {
#ifdef CONFIG_APP_USE_SHT40
    SHT4X_SENSOR_PACKET
#endif
};

typedef struct sensor_reading {
    int err;
    int type;

#ifdef CONFIG_APP_USE_SHT40
    double hum;
    double temp;
#endif

} sensor_reading_t;

#ifdef CONFIG_APP_USE_SHT40
void sht4x_poll(void);
#endif

int sensor_reading_get(void *data, k_timeout_t timeout);

#endif
