#ifndef CONTROL_CORE_STATE_H_
#define CONTROL_CORE_STATE_H_

#include <zephyr/kernel.h>

enum state_codes {
    OK = 0,
    INIT_FAILED,
    DATA_ERROR,
};

struct state {
#ifdef CONFIG_APP_USE_SHT40
    uint8_t sht_state;
    k_timeout_t sht_poll_freq;
    int64_t sht_last_update_tick;

    double sht_hum;
    double sht_temp;
#endif

#ifdef CONFIG_APP_USE_IMU
    uint8_t imu_state;
    k_timeout_t imu_poll_freq;
    int64_t imu_last_update_tick;

    double imu_accel_x;
    double imu_accel_y;
    double imu_accel_z;

    double imu_gyro_x;
    double imu_gyro_y;
    double imu_gyro_z;

    double imu_mag_x;
    double imu_mag_y;
    double imu_mag_z;
#endif

#ifdef CONFIG_HEARTBEAT
    uint8_t heartbeat_state;
#endif

};

#endif /* CONTROL_CORE_STATE_H_ */
