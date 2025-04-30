#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>

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

#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
	const struct device *usb;
	usb = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	if (!device_is_ready(usb) || usb_enable(NULL)) {
		return 0;
	}
#endif

    while (1) {
        k_sleep(K_MSEC(1000));
    }

    return 0;
}
