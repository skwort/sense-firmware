#include "sensors.h"

#ifdef CONFIG_SENSE_CORE_SENSOR_ADCS
#include "adc/adcs.h"
#endif /* CONFIG_SENSE_CORE_SENSOR_ADCS */

#ifdef CONFIG_SENSE_CORE_SENSOR_GNSS
#include "gnss/gnss.h"
#endif /* CONFIG_SENSE_CORE_SENSOR_GNSS */

#ifdef CONFIG_SENSE_CORE_SENSOR_IMU
#include "imu/imu.h"
#endif /* CONFIG_SENSE_CORE_SENSOR_IMU */

#ifdef CONFIG_SENSE_CORE_SENSOR_TEMP_HUM
#include "temp_hum/temp_hum.h"
#endif /* CONFIG_SENSE_CORE_SENSOR_TEMP_HUM */



void sensors_init(void)
{
#ifdef CONFIG_SENSE_CORE_SENSOR_GNSS
    gnss_init();
#endif /* CONFIG_SENSE_CORE_SENSOR_GNSS*/

#ifdef CONFIG_SENSE_CORE_SENSOR_LIS3MDL
    imu_lis3mdl_init();
#endif /* CONFIG_SENSE_CORE_SENSOR_LIS3MDL */

#ifdef CONFIG_SENSE_CORE_SENSOR_LSM6DSO
    imu_lsm6dso_init();
#endif /* CONFIG_SENSE_CORE_SENSOR_LSM6DSO*/

#ifdef CONFIG_SENSE_CORE_SENSOR_SHT4X
    sht4x_init();
#endif /* CONFIG_SENSE_CORE_SENSOR_SHT4X */

#ifdef CONFIG_SENSE_CORE_SENSOR_ADCS
    adcs_init();
#endif /* CONFIG_SENSE_CORE_SENSOR_ADCS*/
}
