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

#ifdef CONFIG_APP_USE_IMU

#if !DT_HAS_COMPAT_STATUS_OKAY(st_lsm6dso)
#error "No st,lsm6dso compatible node found in the device tree"
#endif

#if !DT_HAS_COMPAT_STATUS_OKAY(st_lis3mdl_magn)
#error "No st,lis3mdl compatible node found in the device tree"
#endif

const struct device *const lsm = DEVICE_DT_GET_ANY(st_lsm6dso);
const struct device *const lis = DEVICE_DT_GET_ANY(st_lis3mdl_magn);
#endif

K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_reading), 5, 1);

#ifdef CONFIG_APP_USE_SHT40
int sht4x_init(void)
{
    if (!device_is_ready(sht)) {
		LOG_ERR("Device %s is not ready.", sht->name);
        return 1;
    }

    return 0;
}

void sht4x_poll_work_handler(struct k_work *work)
{
    struct sensor_value temp, hum;
    struct sensor_reading sr = {0};
    int err = 0;

    sr.type = SENSOR_PKT_SHT;

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

#ifdef CONFIG_APP_USE_IMU
int imu_init(void)
{
    if (!device_is_ready(lsm)) {
		LOG_ERR("Device %s is not ready.", lsm->name);
        return 1;
    }

    if (!device_is_ready(lis)) {
		LOG_ERR("Device %s is not ready.", lis->name);
        return 1;
	}

    return 0;
}

/** NOTE: lis3mdl does not support runtime ODR changes. Instead it is
 *  configured via Kconfig CONFIG_LIS3MDL_ODR. I may patch the driver and
 *  add support for this when I get the chance.
 */
int imu_set_sampling_frequency(double freq)
{
    int err = 0;
    struct sensor_value imu_freq;
    sensor_value_from_double(&imu_freq, freq);

    err = sensor_attr_set(lsm,
                          SENSOR_CHAN_ACCEL_XYZ,
                          SENSOR_ATTR_SAMPLING_FREQUENCY,
                          &imu_freq);
    if (err != 0) {
        LOG_ERR("Cannot set accelerometer sampling frequency.");
        return err;
    }

    err = sensor_attr_set(lsm,
                          SENSOR_CHAN_GYRO_XYZ,
                          SENSOR_ATTR_SAMPLING_FREQUENCY,
                          &imu_freq);
    if (err != 0) {
        LOG_ERR("Cannot set gyroscope sampling frequency.");
        return err;
    }

    return 0;
}

void imu_poll_work_handler(struct k_work *work)
{
    struct sensor_value x, y, z;
    struct sensor_reading sr = {0};
    int err = 0;

    sr.type = SENSOR_PKT_IMU;

    err = sensor_sample_fetch_chan(lsm, SENSOR_CHAN_ACCEL_XYZ);
    if (err != 0) {
        LOG_ERR("Failed to fetch accelerometer channel from LSM6DSO device");
        goto fail;
    }

    sensor_channel_get(lsm, SENSOR_CHAN_ACCEL_X, &x);
    sensor_channel_get(lsm, SENSOR_CHAN_ACCEL_Y, &y);
    sensor_channel_get(lsm, SENSOR_CHAN_ACCEL_Z, &z);

    sr.accel_x = sensor_value_to_double(&x);
    sr.accel_y = sensor_value_to_double(&y);
    sr.accel_z = sensor_value_to_double(&z);

    err = sensor_sample_fetch_chan(lsm, SENSOR_CHAN_GYRO_XYZ);
    if (err != 0) {
        LOG_ERR("Failed to fetch gyroscope channel from LSM6DSO device");
        goto fail;
    }

    sensor_channel_get(lsm, SENSOR_CHAN_GYRO_X, &x);
    sensor_channel_get(lsm, SENSOR_CHAN_GYRO_Y, &y);
    sensor_channel_get(lsm, SENSOR_CHAN_GYRO_Z, &z);

    sr.gyro_x = sensor_value_to_double(&x);
    sr.gyro_y = sensor_value_to_double(&y);
    sr.gyro_z = sensor_value_to_double(&z);

    err = sensor_sample_fetch_chan(lis, SENSOR_CHAN_MAGN_XYZ);
    if (err != 0) {
        LOG_ERR("Failed to fetch magnetometer channel from LIS3MDL device");
        goto fail;
    }

    sensor_channel_get(lis, SENSOR_CHAN_MAGN_X, &x);
    sensor_channel_get(lis, SENSOR_CHAN_MAGN_Y, &y);
    sensor_channel_get(lis, SENSOR_CHAN_MAGN_Z, &z);

    sr.mag_x = sensor_value_to_double(&x);
    sr.mag_y = sensor_value_to_double(&y);
    sr.mag_z = sensor_value_to_double(&z);

    k_msgq_put(&sensor_msgq, &sr, K_NO_WAIT);
    return;

fail:
    sr.err = 1;
    k_msgq_put(&sensor_msgq, &sr, K_NO_WAIT);
    return;
}

K_WORK_DEFINE(imu_poll_work, imu_poll_work_handler);

void imu_poll(void)
{
    k_work_submit(&imu_poll_work);
}
#endif


int sensor_reading_get(void *data, k_timeout_t timeout)
{
    return k_msgq_get(&sensor_msgq, data, timeout);
}
