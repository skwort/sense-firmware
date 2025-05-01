#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>

#include <app_version.h>
#include <lib/icmp.h>
#include <lib/heartbeat.h>

LOG_MODULE_REGISTER(main);

#ifdef CONFIG_ICMP
void icmp_test_cb(const uint8_t *payload, size_t payload_len)
{
    LOG_INF("ICMP payload received: %s", payload);
}
#endif /* CONFIG_ICMP */

int main(void)
{
    printk("SENSE Interface Core version %s\n", APP_VERSION_STRING);

#ifdef CONFIG_ICMP
    icmp_register_target(0, icmp_test_cb);
    icmp_init();

    uint8_t payload[] = {'n', 'r', 'f', '5', '3', '\0'};
#endif /* CONFIG_ICMP */

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

#ifdef CONFIG_ICMP
        icmp_notify(0, payload, 6);
#endif /* CONFIG_ICMP */

        k_sleep(K_MSEC(1000));
    }

    return 0;
}
