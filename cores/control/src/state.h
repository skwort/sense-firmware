#ifndef CONTROL_CORE_STATE_H_
#define CONTROL_CORE_STATE_H_

#include <zephyr/kernel.h>

#ifdef CONFIG_APP_USE_GNSS
#include <nrf_modem_gnss.h>
#endif

enum state_codes {
    OK = 0,
    INIT_FAILED,
    DATA_ERROR,
    SLEEPING,
};

struct state {
#ifdef CONFIG_APP_USE_SHT40
    uint8_t sht_state;
    int64_t sht_poll_freq;
    int64_t sht_last_update_tick;

    double sht_hum;
    double sht_temp;
#endif

#ifdef CONFIG_APP_USE_IMU
    uint8_t imu_state;
    int64_t imu_poll_freq;
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

#ifdef CONFIG_APP_USE_GNSS
    uint8_t gnss_state;
    uint16_t gnss_fix_retry_period;
    uint16_t gnss_fix_interval;
    int64_t gnss_last_update_time;

	/** Latitude in degrees. */
	double gnss_latitude;
	/** Longitude in degrees. */
	double gnss_longitude;
	/** Altitude above WGS-84 ellipsoid in meters. */
	float gnss_altitude;
	/** Position accuracy (2D 1-sigma) in meters. */
	float gnss_accuracy;
	/** Altitude accuracy (1-sigma) in meters. */
	float gnss_altitude_accuracy;
	/** Horizontal speed in m/s. */
	float gnss_speed;
	/** Speed accuracy (1-sigma) in m/s. */
	float gnss_speed_accuracy;
	/** Vertical speed in m/s. Positive is up. */
	float gnss_vertical_speed;
	/** Vertical speed accuracy (1-sigma) in m/s. */
	float gnss_vertical_speed_accuracy;
	/** Heading of user movement in degrees. */
	float gnss_heading;
	/** Heading accuracy (1-sigma) in degrees. */
	float gnss_heading_accuracy;
#endif

#ifdef CONFIG_HEARTBEAT
    uint8_t heartbeat_state;
#endif

};

/* Global state variable */
extern struct state system_state;

/* Global state lock */
extern struct k_mutex system_state_mutex;

static inline int state_lock(k_timeout_t timeout)
{
    return k_mutex_lock(&system_state_mutex, timeout);
}

static inline int state_unlock(void)
{
    return k_mutex_unlock(&system_state_mutex);
}

static inline struct state *state_get(void)
{
    return &system_state;
}

#endif /* CONTROL_CORE_STATE_H_ */
