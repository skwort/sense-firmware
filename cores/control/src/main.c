#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app_version.h>

LOG_MODULE_REGISTER(main);

int main(void)
{
    printk("SENSE Control Core version %s\n", APP_VERSION_STRING);
    return 0;
}
 