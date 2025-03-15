#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <app_version.h>
#include <zephyr/drivers/sensor/sht4x.h>

#include "sensors.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void)
{
	printk("SENSE Control Core version %s\n", APP_VERSION_STRING);

    while (1) {
        struct sensor_reading sr = {0};
        int err = sensor_reading_get(&sr, K_NO_WAIT);
        if (!err) {
            if (sr.err)
                printf("Error getitng sensor values\n");
#ifdef CONFIG_APP_USE_SHT40
            else if (sr.type == SHT4X_SENSOR_PACKET)
                printf("SHT40: Temp: %.2f C  RH: %0.2f %%\n",
                       sr.temp, sr.hum);
#endif
        }

#ifdef CONFIG_APP_USE_SHT40
        sht4x_poll();
#endif

        k_sleep(K_MSEC(1000));
    }

    return 0;
}
