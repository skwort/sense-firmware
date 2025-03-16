#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app_version.h>
#include <lib/heartbeat.h>

LOG_MODULE_REGISTER(main);

int main(void)
{
    printk("SENSE Interface Core version %s\n", APP_VERSION_STRING);

#ifdef CONFIG_HEARTBEAT
    if (heartbeat_init_start(K_MSEC(CONFIG_HEARTBEAT_DEFAULT_DURATION)))
        LOG_ERR("Failed to initialise heartbeat.");
#endif

    while (1) {
        k_sleep(K_MSEC(1000));
    }

    return 0;
}
