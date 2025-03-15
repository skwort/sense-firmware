#include "zephyr/sys_clock.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "sensors.h"

LOG_MODULE_REGISTER(sensors, CONFIG_APP_LOG_LEVEL);

#ifdef CONFIG_APP_USE_SHT40
#include <zephyr/drivers/sensor/sht4x.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif

const struct device *const sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
#endif

K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_reading), 5, 1);

#ifdef CONFIG_APP_USE_SHT40

void sht4x_poll_work_handler(struct k_work *work)
{
    struct sensor_value temp, hum;
    struct sensor_reading sr = {0};
    int err = 0;

    if (!device_is_ready(sht)) {
		LOG_ERR("Device %s is not ready.", sht->name);
        goto fail;
    }

    err = sensor_sample_fetch(sht);
    if (err != 0) {
        LOG_ERR("Failed to fetch sample from SHT4X device");
        goto fail;
    }

    sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum);

    sr.temp = sensor_value_to_double(&temp),
    sr.hum = sensor_value_to_double(&hum);

    k_msgq_put(&sensor_msgq, &sr, K_NO_WAIT);
    return;

fail:
    sr.err = 1;
    k_msgq_put(&sensor_msgq, &sr, K_NO_WAIT);
    return;
}

K_WORK_DEFINE(sht4x_poll_work, sht4x_poll_work_handler);

void sht4x_poll(void)
{
    k_work_submit(&sht4x_poll_work);
}
#endif

int sensor_reading_get(void *data, k_timeout_t timeout)
{
    return k_msgq_get(&sensor_msgq, data, timeout);
}
