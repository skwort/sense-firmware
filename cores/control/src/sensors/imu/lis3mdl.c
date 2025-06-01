#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include <datapoint_helpers.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(st_lis3mdl_magn)
#error "No st,lis3mdl compatible node found in the device tree"
#endif

LOG_MODULE_DECLARE(imu);

const struct device *const lis3mdl = DEVICE_DT_GET_ANY(st_lis3mdl_magn);

static struct k_work_delayable lis3mdl_poll_work;
static atomic_t poll_interval_ms = 500;

void lis3mdl_poll_work_handler(struct k_work *work)
{
    int err = 0;

    err = sensor_sample_fetch(lis3mdl);
    if (err != 0) {
        LOG_ERR("Failed to fetch samples from LIS3MDL");
        goto reschedule;
    }

    err = get_and_submit_sensor_datapoint(lis3mdl,
                                        SENSOR_CHAN_MAGN_X,
                                        "lis3mdl_mag_x",
                                        "G");
    if (err != 0) {
        LOG_ERR("Failed to get magnetometer X from LIS3MDL");
    }

    err = get_and_submit_sensor_datapoint(lis3mdl,
                                        SENSOR_CHAN_MAGN_Y,
                                        "lis3mdl_mag_y",
                                        "G");
    if (err != 0) {
        LOG_ERR("Failed to get magnetometer Y from LIS3MDL");
    }

    err = get_and_submit_sensor_datapoint(lis3mdl,
                                        SENSOR_CHAN_MAGN_Z,
                                        "lis3mdl_mag_z",
                                        "G");
    if (err != 0) {
        LOG_ERR("Failed to get magnetometer Z from LIS3MDL");
    }

reschedule:
    k_work_schedule(&lis3mdl_poll_work, K_MSEC(atomic_get(&poll_interval_ms)));
}

int imu_lis3mdl_init(void)
{
    if (!device_is_ready(lis3mdl)) {
		LOG_ERR("Device %s is not ready.", lis3mdl->name);
        return 1;
	}

    k_work_init_delayable(&lis3mdl_poll_work, lis3mdl_poll_work_handler);
    k_work_schedule(&lis3mdl_poll_work, K_NO_WAIT);

    return 0;
}

void lis3mdl_set_poll_interval(int32_t interval_ms)
{
    atomic_set(&poll_interval_ms, interval_ms);
    k_work_reschedule(&lis3mdl_poll_work, K_MSEC(interval_ms));
}
