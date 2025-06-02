
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "sdi12_queue.h"

/* Thread parameters */
#define SDI12_STACKSIZE 1024
#define SDI12_PRIORITY  4

LOG_MODULE_REGISTER(sdi12, LOG_LEVEL_INF);

#define UART_RX_TIMEOUT (20 * USEC_PER_MSEC)

#define SDI12_UART_NODE DT_NODELABEL(sdi12_uart)
static const struct device *sdi12_uart = DEVICE_DT_GET(SDI12_UART_NODE);

#define SDI12_GPIOS_NODE DT_NODELABEL(sdi12_gpios)
static const struct gpio_dt_spec sdi12_space = GPIO_DT_SPEC_GET(SDI12_GPIOS_NODE, not_space_gpio);
static const struct gpio_dt_spec sdi12_tx_en = GPIO_DT_SPEC_GET(SDI12_GPIOS_NODE, tx_en_gpio);
static const struct gpio_dt_spec sdi12_rx_en = GPIO_DT_SPEC_GET(SDI12_GPIOS_NODE, rx_en_gpio);

#define PWR_12V_NODE DT_NODELABEL(ls_12v)
static const struct gpio_dt_spec ls_12v = GPIO_DT_SPEC_GET(PWR_12V_NODE, gpios);

K_SEM_DEFINE(tx_sem, 0, 1);
K_SEM_DEFINE(rx_sem, 0, 1);

static uint8_t uart_recv_buffer[SDI12_MSG_LEN] = {0};
static uint8_t uart_send_buffer[SDI12_MSG_LEN] = {0};


static int sdi12_gpio_init(void)
{
    if (!gpio_is_ready_dt(&sdi12_space)) {
        LOG_ERR("SDI-12 SPACE GPIO is not ready.");
        return -ENXIO;
    }
    if (!gpio_is_ready_dt(&sdi12_tx_en)) {
        LOG_ERR("SDI-12 TX_EN GPIO is not ready.");
        return -ENXIO;
    }
    if (!gpio_is_ready_dt(&sdi12_rx_en)) {
        LOG_ERR("SDI-12 RX_EN GPIO is not ready.");
        return -ENXIO;
    }

    if (!gpio_is_ready_dt(&ls_12v)) {
        LOG_ERR("12V_EN GPIO is not ready.");
        return -ENXIO;
    }

    int err;

    /* Initialise 12V to off state */
    err = gpio_pin_configure_dt(&ls_12v, GPIO_OUTPUT_ACTIVE);
	if (err < 0) {
		return err;
	}

    /* No spacing by default (idle line) */
    err = gpio_pin_configure_dt(&sdi12_space, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		return err;
	}

    /* Enable TX */
    err = gpio_pin_configure_dt(&sdi12_tx_en, GPIO_OUTPUT_ACTIVE);
	if (err < 0) {
		return err;
	}

    /* Keep RX disabled */
    err = gpio_pin_configure_dt(&sdi12_rx_en, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		return err;
	}

    return 0;
}

static void sdi12_uart_cb(const struct device *dev, struct uart_event *evt,
                          void *user_data)
{
    switch (evt->type) {

    case UART_TX_DONE:
        k_sem_give(&tx_sem);

        // Swap to RX mode
        gpio_pin_set_dt(&sdi12_tx_en, 0);
        gpio_pin_set_dt(&sdi12_rx_en, 1);

        break;

    case UART_RX_RDY:
        break;

    case UART_RX_DISABLED:
        break;

    case UART_RX_STOPPED:
        break;

    case UART_RX_BUF_REQUEST:
        break;

    case UART_RX_BUF_RELEASED:
        k_sem_give(&rx_sem);
        break;

    default:
        break;
    }
}

/* TODO: Migrate to state machine model with retries */
void sdi12_thread(void)
{
    int ret = 0;

    if (sdi12_gpio_init()) {
        LOG_ERR("Failed to init SDI-12 GPIOs");
        return;
    }

    ret = uart_callback_set(sdi12_uart, sdi12_uart_cb, NULL);
    if (ret) {
        LOG_ERR("SDI-12 callback set failed: %d", ret);
        return;
    }

    bool msg_in_flight = false;
    while (1) {
        if (!msg_in_flight) {

            sdi12_command_dequeue(uart_send_buffer, K_FOREVER);

            /* Swap to TX mode */
            gpio_pin_set_dt(&sdi12_tx_en, 1);
            gpio_pin_set_dt(&sdi12_rx_en, 0);

            /* Clear the receive buffer */
            memset(uart_recv_buffer, 0, sizeof(uart_recv_buffer));

            /* Pre-enable reception */
            uart_rx_enable(sdi12_uart, uart_recv_buffer,
                           sizeof(uart_recv_buffer), UART_RX_TIMEOUT);

            LOG_DBG("Marking SDI-12 line.");
            gpio_pin_set_dt(&sdi12_space, 1);
            k_sleep(K_MSEC(12));

            LOG_DBG("Marking SDI-12 line.");
            gpio_pin_set_dt(&sdi12_space, 0);
            k_sleep(K_MSEC(9));

            LOG_INF("Transmitting SDI-12 packet.");
            ret = uart_tx(sdi12_uart,
                        uart_send_buffer,
                        strlen(uart_send_buffer),
                        SYS_FOREVER_US);
            if (ret) {
                LOG_ERR("SDI-12 transmission failed: %d", ret);
                continue;
            }

            /* Wait for transmission to complete */
            k_sem_take(&tx_sem, K_FOREVER);
            msg_in_flight = true;

        } else {

            LOG_INF("Waiting for SDI-12 packet.");
            ret = k_sem_take(&rx_sem, K_MSEC(100));
            if (ret == -EAGAIN) {
                LOG_INF("SDI-12 reception timed out.");
                uart_rx_disable(sdi12_uart);
                msg_in_flight = false;
                k_sem_take(&rx_sem, K_FOREVER);
            } else if (ret == 0) {
                LOG_INF("Received SDI-12 packet: %s", uart_recv_buffer);
                msg_in_flight = false;
                sdi12_response_enqueue(uart_recv_buffer, K_NO_WAIT);

                /* Clear the send buffer */
                memset(uart_send_buffer, 0, sizeof(uart_send_buffer));
            } else {
                LOG_ERR("Sempahore error.");
            }

            k_sleep(K_MSEC(5));
        }
    }
}

/* Register the thread */
K_THREAD_DEFINE(sdi12, SDI12_STACKSIZE, sdi12_thread, NULL, NULL,
                NULL, SDI12_PRIORITY, 0, 0);
