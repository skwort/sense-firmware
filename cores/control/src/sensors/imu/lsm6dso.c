
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include <datapoint_helpers.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(st_lsm6dso)
#error "No st,lsm6dso compatible node found in the device tree"
#endif

LOG_MODULE_DECLARE(imu);

const struct device *const lsm6dso = DEVICE_DT_GET_ANY(st_lsm6dso);

static struct k_work_delayable lsm6dso_poll_work;
static atomic_t poll_interval_ms = 3000;

int lsm6dso_set_sampling_frequency(double freq)
{
    int err = 0;
    struct sensor_value imu_freq;
    sensor_value_from_double(&imu_freq, freq);

    err = sensor_attr_set(lsm6dso,
                          SENSOR_CHAN_ACCEL_XYZ,
                          SENSOR_ATTR_SAMPLING_FREQUENCY,
                          &imu_freq);
    if (err != 0) {
        LOG_ERR("Cannot set accelerometer sampling frequency.");
        return err;
    }

    err = sensor_attr_set(lsm6dso,
                          SENSOR_CHAN_GYRO_XYZ,
                          SENSOR_ATTR_SAMPLING_FREQUENCY,
                          &imu_freq);
    if (err != 0) {
        LOG_ERR("Cannot set gyroscope sampling frequency.");
        return err;
    }

    return 0;
}

void lsm6dso_poll_work_handler(struct k_work *work)
{
    int err = 0;

    err = sensor_sample_fetch_chan(lsm6dso, SENSOR_CHAN_ACCEL_XYZ);
    if (err != 0) {
        LOG_ERR("Failed to fetch accelerometer channel from LSM6DSO device");
        goto gyro;
    }

    err = get_and_submit_sensor_datapoint(lsm6dso,
                                          SENSOR_CHAN_ACCEL_X,
                                          "lsm6dso_accel_x",
                                          "m/s^2");
    if (err != 0) {
        LOG_ERR("Failed to get accelerometer X from LSM6DSO");
    }

    err = get_and_submit_sensor_datapoint(lsm6dso,
                                          SENSOR_CHAN_ACCEL_Y,
                                          "lsm6dso_accel_y",
                                          "m/s^2");
    if (err != 0) {
        LOG_ERR("Failed to get accelerometer Y from LSM6DSO");
    }

    err = get_and_submit_sensor_datapoint(lsm6dso,
                                          SENSOR_CHAN_ACCEL_Z,
                                          "lsm6dso_accel_z",
                                          "m/s^2");
    if (err != 0) {
        LOG_ERR("Failed to get accelerometer Z from LSM6DSO");
    }

gyro:
    err = sensor_sample_fetch_chan(lsm6dso, SENSOR_CHAN_GYRO_XYZ);
    if (err != 0) {
        LOG_ERR("Failed to fetch gyroscope channel from LSM6DSO");
        goto reschedule;
    }

    err = get_and_submit_sensor_datapoint(lsm6dso,
                                          SENSOR_CHAN_GYRO_X,
                                          "lsm6dso_gyro_x",
                                          "rad/s");
    if (err != 0) {
        LOG_ERR("Failed to get gyroscope X from LSM6DSO");
    }

    err = get_and_submit_sensor_datapoint(lsm6dso,
                                          SENSOR_CHAN_GYRO_Y,
                                          "lsm6dso_gyro_y",
                                          "rad/s");
    if (err != 0) {
        LOG_ERR("Failed to get gyroscope Y from LSM6DSO");
    }

    err = get_and_submit_sensor_datapoint(lsm6dso,
                                          SENSOR_CHAN_GYRO_Z,
                                          "lsm6dso_gyro_z",
                                          "rad/s");
    if (err != 0) {
        LOG_ERR("Failed to get gyroscope Z from LSM6DSO");
    }

reschedule:
    k_work_schedule(&lsm6dso_poll_work, K_MSEC(atomic_get(&poll_interval_ms)));
}

int imu_lsm6dso_init(void)
{
    if (!device_is_ready(lsm6dso)) {
		LOG_ERR("Device %s is not ready.", lsm6dso->name);
        return 1;
	}

    lsm6dso_set_sampling_frequency(2.0);

    k_work_init_delayable(&lsm6dso_poll_work, lsm6dso_poll_work_handler);
    k_work_schedule(&lsm6dso_poll_work, K_NO_WAIT);

    return 0;
}

void lsm6dso_set_poll_interval(int32_t interval_ms)
{
    atomic_set(&poll_interval_ms, interval_ms);
    k_work_reschedule(&lsm6dso_poll_work, K_MSEC(interval_ms));
}
