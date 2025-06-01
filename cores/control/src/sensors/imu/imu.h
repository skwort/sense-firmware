#ifndef SENSORS_IMU_H_
#define SENSORS_IMU_H_

#ifdef CONFIG_SENSE_CORE_SENSOR_LIS3MDL

int imu_lis3mdl_init(void);

#endif /* CONFIG_SENSE_CORE_SENSOR_LIS3MDL */

#ifdef CONFIG_SENSE_CORE_SENSOR_LSM6DSO

int imu_lsm6dso_init(void);

#endif /* CONFIG_SENSE_CORE_SENSOR_LSM6DSO */

#endif /* SENSORS_IMU_H_ */
