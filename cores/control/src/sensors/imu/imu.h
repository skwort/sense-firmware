#ifndef SENSORS_IMU_H_
#define SENSORS_IMU_H_

#include <stdint.h>

#ifdef CONFIG_SENSE_CORE_SENSOR_LIS3MDL

int imu_lis3mdl_init(void);

void lis3mdl_set_poll_interval(int32_t interval_ms);

#endif /* CONFIG_SENSE_CORE_SENSOR_LIS3MDL */

#ifdef CONFIG_SENSE_CORE_SENSOR_LSM6DSO

int imu_lsm6dso_init(void);

void lsm6dso_set_poll_interval(int32_t interval_ms);

#endif /* CONFIG_SENSE_CORE_SENSOR_LSM6DSO */

#endif /* SENSORS_IMU_H_ */
