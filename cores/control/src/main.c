#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <app_version.h>
#include <zephyr/drivers/sensor/sht4x.h>

#include <lib/heartbeat.h>
#include "sensors.h"
#include "state.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

void poll_sensors(struct state *state)
{
    struct sensor_reading sr = {0};
    sr.err = 1;

    /* Attempt to get sensor readings */
    while (sensor_reading_get(&sr, K_NO_WAIT) == 0) {
        if (sr.err)
            LOG_ERR("Error getitng sensor values\n");

#ifdef CONFIG_APP_USE_SHT40
        else if (sr.type == SENSOR_PKT_SHT) {
            state->sht_temp = sr.temp;
            state->sht_hum = sr.hum;
            LOG_INF("SHT state updated.");
            LOG_DBG("SHT40: Temp: %.2f C  RH: %0.2f %%\n",
                    sr.temp, sr.hum);
        }
#endif

#ifdef CONFIG_APP_USE_IMU
        else if (sr.type ==  SENSOR_PKT_IMU) {
            state->imu_accel_x = sr.accel_x;
            state->imu_accel_y = sr.accel_y;
            state->imu_accel_z = sr.accel_z;
            state->imu_gyro_x = sr.gyro_x;
            state->imu_gyro_y = sr.gyro_y;
            state->imu_gyro_z = sr.gyro_z;
            state->imu_mag_x = sr.mag_x;
            state->imu_mag_y = sr.mag_y;
            state->imu_mag_z = sr.mag_z;
            LOG_INF("IMU state updated.");
            LOG_DBG("IMU: Accel: x = %.2f  y = %.2f  z = %.2f\n"
                    "      Gyro: x = %.2f  y = %.2f  z = %.2f\n"
                    "       Mag: x = %.2f  y = %.2f  z = %.2f\n",
                    sr.accel_x, sr.accel_y, sr.accel_z,
                    sr.gyro_x, sr.gyro_y, sr.gyro_z,
                    sr.mag_x, sr.mag_y, sr.mag_z);

        }
#endif
    }

#ifdef CONFIG_APP_USE_SHT40
    /* Poll the SHT sensor if it is working and the specified poll duration has
     * passed. Note that the state->sht_last_update_tick corresponds to the
     * last requested update time (subbmission to work queue), and not the time
     * the polling work item was fulfilled */
    if (state->sht_state == OK &&
        k_uptime_get() - state->sht_last_update_tick >= state->sht_poll_freq) {
        LOG_INF("Polling SHT");
        state->sht_last_update_tick = k_uptime_get();
        sht4x_poll();
    }
#endif

#ifdef CONFIG_APP_USE_IMU
    /* Poll the IMU sensors if they are working and the specified poll duration
     * has passed. Note that the state->imu_last_update_tick corresponds to the
     * last requested update time (subbmission to work queue), and not the time
     * the polling work item was fulfilled */
    if (state->imu_state == OK &&
        k_uptime_get() - state->imu_last_update_tick >= state->imu_poll_freq) {
        LOG_INF("Polling IMU");
        state->imu_last_update_tick = k_uptime_get();
        imu_poll();
    }
#endif
}

int main(void)
{
	printk("SENSE Control Core version %s\n", APP_VERSION_STRING);

    struct state state = {0};

#ifdef CONFIG_HEARTBEAT
    /* Initialise and start the heartbeat */
    if (heartbeat_init_start(K_MSEC(CONFIG_HEARTBEAT_DEFAULT_DURATION))) {
        LOG_ERR("Failed to initialise heartbeat.");
        state.heartbeat_state = INIT_FAILED;
    }
    state.heartbeat_state = OK;

#endif

#ifdef CONFIG_APP_USE_IMU
    /* Initialise the IMU */
    if (imu_init() != 0) {
        state.imu_state = INIT_FAILED;
    } else {
        state.imu_state = OK;
        state.imu_poll_freq = CONFIG_APP_SENSOR_DEFAULT_POLL_FREQ;
        imu_set_sampling_frequency(2.0);
    }

#endif

#ifdef CONFIG_APP_USE_SHT40
    /* Initialise the SHT40 */
    if (sht4x_init() != 0)
        state.sht_state = INIT_FAILED;
    else {
        state.sht_state = OK;
        state.sht_poll_freq = CONFIG_APP_SENSOR_DEFAULT_POLL_FREQ;
    }
#endif

    while (1) {
        poll_sensors(&state);
        k_sleep(K_MSEC(500));
    }

    return 0;
}
