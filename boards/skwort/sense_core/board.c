#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>

#define SUPPLY_OFF 0
#define SUPPLY_ON  1

static int set_supply(const struct gpio_dt_spec *gpio, int value)
{
    int rv = -ENODEV;

    if (gpio_is_ready_dt(gpio)) {
        gpio_pin_configure_dt(gpio, GPIO_OUTPUT_INACTIVE);
        gpio_pin_set_dt(gpio, value);
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
    rc = set_supply(&ls_sensors, SUPPLY_ON);
    if (rc < 0) {
        printk("3V3 sensors supply not enabled: %d\n", rc);
    }
#endif /* DT_NODE_HAS_STATUS_OKAY(SENSORS_NODE) */

#define SD_CARD_NODE DT_NODELABEL(ls_sdcard)
#if DT_NODE_HAS_STATUS_OKAY(SD_CARD_NODE)
static const struct gpio_dt_spec ls_sdcard =
    GPIO_DT_SPEC_GET(SD_CARD_NODE, gpios);
    /* Enable the SD card power domain */
    rc = set_supply(&ls_sdcard, SUPPLY_OFF);
    if (rc < 0) {
        printk("SD card supply not enabled: %d\n", rc);
    }
#endif /* DT_NODE_HAS_STATUS_OKAY(SD_CARD_NODE) */

#define SPIF_NODE DT_NODELABEL(ls_spiflash)
#if DT_NODE_HAS_STATUS_OKAY(SPIF_NODE)
static const struct gpio_dt_spec ls_spiflash =
    GPIO_DT_SPEC_GET(SPIF_NODE, gpios);
    /* Enable the SPI flash power domain */
    rc = set_supply(&ls_spiflash, SUPPLY_ON);
    if (rc < 0) {
        printk("SPI Flash supply not enabled: %d\n", rc);
    }
#endif /* DT_NODE_HAS_STATUS_OKAY(SPIF_NODE) */

#define PWR_5VL_NODE DT_NODELABEL(ls_5vl)
#if DT_NODE_HAS_STATUS_OKAY(PWR_5VL_NODE)
static const struct gpio_dt_spec ls_5vl =
    GPIO_DT_SPEC_GET(PWR_5VL_NODE, gpios);
    /* Enable the 5VL power domain */
    rc = set_supply(&ls_5vl, SUPPLY_ON);
    if (rc < 0) {
        printk("5VL supply set failed: %d\n", rc);
    }
#endif /* DT_NODE_HAS_STATUS_OKAY(PWR_5VL_NODE) */

#define PWR_5VH_NODE DT_NODELABEL(ls_5vh)
#if DT_NODE_HAS_STATUS_OKAY(PWR_5VH_NODE)
static const struct gpio_dt_spec ls_5vh =
    GPIO_DT_SPEC_GET(PWR_5VH_NODE, gpios);
    /* Enable the 5VH power domain */
    rc = set_supply(&ls_5vh, SUPPLY_OFF);
    if (rc < 0) {
        printk("5VH supply set failed: %d\n", rc);
    }
#endif /* DT_NODE_HAS_STATUS_OKAY(PWR_5VH_NODE) */
#endif /* CONFIG_BOARD_SENSE_CORE_NRF9161 || NRF9161_NS */

    /* Give the power-rails time to stabilise */
    k_msleep(10);

    return rc;
}

/* Initialise after GPIO driver */
SYS_INIT(sense_core_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
