#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>


static int enable_supply(const struct gpio_dt_spec *gpio)
{
    int rv = -ENODEV;

    if (gpio_is_ready_dt(gpio)) {
        gpio_pin_configure_dt(gpio, GPIO_OUTPUT_ACTIVE);
        gpio_pin_set_dt(gpio, 1);
        rv = 0;
    }

    return rv;
}

static int sense_core_init(void)
{
    int rc = 0;

#if defined(CONFIG_BOARD_SENSE_CORE_NRF9161) || \
    defined(CONFIG_BOARD_SENSE_CORE_NRF9161_NS)
    /* Initialise all peripherals on boot */
#define SENSORS_NODE DT_NODELABEL(ls_sensors)
#if DT_NODE_HAS_STATUS_OKAY(SENSORS_NODE)
static const struct gpio_dt_spec ls_sensors =
    GPIO_DT_SPEC_GET(SENSORS_NODE, gpios);
    /* Enable the 3V3_sensors power domain */
    rc = enable_supply(&ls_sensors);
    if (rc < 0) {
        printk("3V3 sensors supply not enabled: %d\n", rc);
    }
#endif /* DT_NODE_HAS_STATUS_OKAY(SENSORS_NODE) */

#define SD_CARD_NODE DT_NODELABEL(ls_sdcard)
#if DT_NODE_HAS_STATUS_OKAY(SD_CARD_NODE)
static const struct gpio_dt_spec ls_sdcard =
    GPIO_DT_SPEC_GET(SD_CARD_NODE, gpios);
    /* Enable the SD card power domain */
    rc = enable_supply(&ls_sdcard);
    if (rc < 0) {
        printk("SD card supply not enabled: %d\n", rc);
    }
#endif /* DT_NODE_HAS_STATUS_OKAY(SD_CARD_NODE) */

#define SPIF_NODE DT_NODELABEL(ls_spiflash)
#if DT_NODE_HAS_STATUS_OKAY(SPIF_NODE)
static const struct gpio_dt_spec ls_spiflash =
    GPIO_DT_SPEC_GET(SPIF_NODE, gpios);
    /* Enable the SPI flash power domain */
    rc = enable_supply(&ls_spiflash);
    if (rc < 0) {
        printk("SPI Flash supply not enabled: %d\n", rc);
    }
#endif /* DT_NODE_HAS_STATUS_OKAY(SPIF_NODE) */
#endif /* CONFIG_BOARD_SENSE_CORE_NRF9161 || NRF9161_NS */

    return rc;
}

/* Initialise after GPIO driver */
SYS_INIT(sense_core_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
