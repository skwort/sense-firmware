#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <app_version.h>
#include <zephyr/drivers/sensor/sht4x.h>

#include <lib/heartbeat.h>
#include "sensors.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void)
{
	printk("SENSE Control Core version %s\n", APP_VERSION_STRING);

#ifdef CONFIG_HEARTBEAT
    if (heartbeat_init_start(K_MSEC(CONFIG_HEARTBEAT_DEFAULT_DURATION)))
        LOG_ERR("Failed to initialise heartbeat.");
#endif

#ifdef CONFIG_APP_USE_IMU
    imu_init();
    imu_set_sampling_frequency(2.0);
#endif

#ifdef CONFIG_APP_USE_SHT40
    sht4x_init();
#endif

    while (1) {
        struct sensor_reading sr = {0};
        sr.err = 1;

        while (sensor_reading_get(&sr, K_NO_WAIT) == 0) {
            if (sr.err)
                printf("Error getitng sensor values\n");

#ifdef CONFIG_APP_USE_SHT40
            else if (sr.type == SENSOR_PKT_SHT)
                printf("SHT40: Temp: %.2f C  RH: %0.2f %%\n",
                       sr.temp, sr.hum);
#endif

#ifdef CONFIG_APP_USE_IMU
            else if (sr.type ==  SENSOR_PKT_IMU)
                printf("IMU: Accel: x = %.2f  y = %.2f  z = %.2f\n"
                       "      Gyro: x = %.2f  y = %.2f  z = %.2f\n"
                       "       Mag: x = %.2f  y = %.2f  z = %.2f\n",
                       sr.accel_x, sr.accel_y, sr.accel_z,
                       sr.gyro_x, sr.gyro_y, sr.gyro_z,
                       sr.mag_x, sr.mag_y, sr.mag_z);
#endif
        }

#ifdef CONFIG_APP_USE_SHT40
        sht4x_poll();
#endif

#ifdef CONFIG_APP_USE_IMU
        imu_poll();
#endif

        k_sleep(K_MSEC(1000));
    }

    return 0;
}
