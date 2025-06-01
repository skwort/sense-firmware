#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <app_version.h>
#include <zephyr/drivers/sensor/sht4x.h>

#include <lib/icmp.h>
#include <lib/heartbeat.h>
#include <lib/cellular.h>

#include "state.h"

#ifdef CONFIG_SENSE_CORE_SENSORS
#include "sensors/sensors.h"
#endif /* CONFIG_SENSE_CORE_SENSORS */

LOG_MODULE_REGISTER(main, LOG_LEVEL_ERR);

#ifdef CONFIG_ICMP
void icmp_test_cb(const uint8_t *payload, size_t payload_len)
{
    LOG_INF("ICMP payload received: %s", payload);
}
#endif /* CONFIG_ICMP */

void cellular_recv_cb(const uint8_t *payload, size_t payload_len)
{
    LOG_INF("Payload recvd: %d", payload_len);
}

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

    state_unlock();

#ifdef CONFIG_SENSE_CORE_SENSORS
    sensors_init();
#endif

#ifdef CONFIG_CELLULAR
    if (cellular_init(cellular_recv_cb) != 0) {
        LOG_ERR("Cellular init failed");
    }
#endif

#ifdef CONFIG_ICMP
    icmp_register_target(0, icmp_test_cb);
    icmp_init();

    uint8_t payload[6] = {'n', 'r', 'f', '9', '1', '\0'};
#endif /* CONFIG_ICMP */

    while (1) {

#ifdef CONFIG_ICMP
        /* Send icmp notification */
        icmp_notify(0, payload, 6);
#endif /* CONFIG_ICMP */

        k_sleep(K_MSEC(1000));
    }

    return 0;
}
