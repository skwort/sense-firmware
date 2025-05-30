#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <app_version.h>
#include <zephyr/drivers/sensor/sht4x.h>

#include <lib/icmp.h>
#include <lib/heartbeat.h>
#include "gnss.h"
#include "sensors.h"
#include "state.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#ifdef CONFIG_ICMP
void icmp_test_cb(const uint8_t *payload, size_t payload_len)
{
    LOG_INF("ICMP payload received: %s", payload);
}
#endif /* CONFIG_ICMP */

int main(void)
{
	printk("SENSE Control Core version %s\n", APP_VERSION_STRING);

    struct state *state = state_get();

    state_lock(K_FOREVER);

#ifdef CONFIG_HEARTBEAT
    /* Initialise and start the heartbeat */
    if (heartbeat_init_start(K_MSEC(CONFIG_HEARTBEAT_DEFAULT_DURATION))) {
        LOG_ERR("Failed to initialise heartbeat.");
        state->heartbeat_state = INIT_FAILED;
    }
    state->heartbeat_state = OK;

#endif /* CONFIG_HEARTBEAT */

#ifdef CONFIG_APP_USE_GNSS
    if (gnss_init(state) != 0)
        state->gnss_state = INIT_FAILED;
    else
        state->gnss_state = OK;
#endif /* CONFIG_APP_USE_GNSS */

#ifdef CONFIG_APP_USE_IMU
    /* Initialise the IMU */
    if (imu_init() != 0) {
        state->imu_state = INIT_FAILED;
    } else {
        state->imu_state = OK;
        state->imu_poll_freq = CONFIG_APP_SENSOR_DEFAULT_POLL_FREQ;
        imu_set_sampling_frequency(2.0);
    }

#endif /* CONFIG_APP_USE_IMU */

#ifdef CONFIG_APP_USE_SHT40
    /* Initialise the SHT40 */
    if (sht4x_init() != 0)
        state->sht_state = INIT_FAILED;
    else {
        state->sht_state = OK;
        state->sht_poll_freq = CONFIG_APP_SENSOR_DEFAULT_POLL_FREQ;
    }
#endif /* CONFIG_APP_USE_SHT40 */

    state_unlock();

#ifdef CONFIG_ICMP
    icmp_register_target(0, icmp_test_cb);
    icmp_init();

    uint8_t payload[6] = {'n', 'r', 'f', '9', '1', '\0'};
#endif /* CONFIG_ICMP */


    while (1) {
        state_lock(K_FOREVER);

#ifdef CONFIG_APP_USE_GNSS
        poll_gnss(state);
#endif /* CONFIG_APP_USE_GNSS */

#ifdef CONFIG_APP_USE_SENSORS
        poll_sensors(state);
#endif /* CONFIG_APP_USE_SENSORS */

        state_unlock();

#ifdef CONFIG_ICMP
        /* Send icmp notification */
        icmp_notify(0, payload, 6);
#endif /* CONFIG_ICMP */

        k_sleep(K_MSEC(1000));
    }

    return 0;
}
