#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/sht4x.h>

#include <datapoint_helpers.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif

LOG_MODULE_REGISTER(temp_hum, CONFIG_SENSE_CORE_SENSOR_TEMP_HUM_LOG_LEVEL);

const struct device *const sht4x = DEVICE_DT_GET_ANY(sensirion_sht4x);

static struct k_work_delayable sht4x_poll_work;
static atomic_t poll_interval_ms = 500;


void sht4x_poll_work_handler(struct k_work *work)
{
    int err = 0;

    err = sensor_sample_fetch(sht4x);
    if (err != 0) {
        LOG_ERR("Failed to fetch sample from SHT4X device");
        goto reschedule;
    }

    err = get_and_submit_sensor_datapoint(sht4x,
                                        SENSOR_CHAN_AMBIENT_TEMP,
                                        "sht4x_temp",
                                        "deg C");
    if (err != 0) {
        LOG_ERR("Failed to get temperature from SHT4X");
    }

    err = get_and_submit_sensor_datapoint(sht4x,
                                        SENSOR_CHAN_HUMIDITY,
                                        "sht4x_hum",
                                        "%");
    if (err != 0) {
        LOG_ERR("Failed to get humidity from SHT4X");
    }

reschedule:
    k_work_schedule(&sht4x_poll_work, K_MSEC(atomic_get(&poll_interval_ms)));
}

int sht4x_init(void)
{
    if (!device_is_ready(sht4x)) {
		LOG_ERR("Device %s is not ready.", sht4x->name);
        return 1;
    }

    k_work_init_delayable(&sht4x_poll_work, sht4x_poll_work_handler);
    k_work_schedule(&sht4x_poll_work, K_NO_WAIT);

    return 0;
}

void sht4x_set_poll_interval(int32_t interval_ms)
{
    atomic_set(&poll_interval_ms, interval_ms);
    k_work_reschedule(&sht4x_poll_work, K_MSEC(interval_ms));
}
