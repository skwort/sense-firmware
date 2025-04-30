#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <lib/icmp.h>

#include "icmp_frame.h"
#include "icmp_queue.h"
#include "icmp_phy.h"

LOG_MODULE_REGISTER(icmp_phy_uart);

static const struct device *icmp_uart = DEVICE_DT_GET(DT_NODELABEL(icmp_uart));

static uint8_t icmp_rx_buf[ICMP_MAX_FRAME_SIZE] = {0};

/* Timeout for UART DMA */
#define ICMP_UART_RX_TIMEOUT (10 * USEC_PER_MSEC)


/* UART callback prototype */
static void icmp_uart_cb(const struct device *dev,
                         struct uart_event *evt,
                         void *user_data)
{
    int ret = 0;
    struct icmp_frame *frame = NULL;

    switch (evt->type) {
    case UART_TX_DONE:
        break;

    case UART_RX_RDY:
        break;

    case UART_RX_DISABLED:
        uart_rx_enable(icmp_uart,
                      icmp_rx_buf,
                      sizeof(icmp_rx_buf),
                      ICMP_UART_RX_TIMEOUT);
        break;

    case UART_RX_STOPPED:
        break;

    case UART_RX_BUF_REQUEST:
        break;

    case UART_RX_BUF_RELEASED:

        ret = icmp_frame_alloc(&frame);
        if (ret != 0) {
            LOG_ERR("Failed to allocate memory for ICMP frame.");
            break;
        }
        ret = icmp_frame_unpack(frame, icmp_rx_buf, sizeof(icmp_rx_buf));
        if (ret != 0) {
            LOG_ERR("Failed to unpack ICMP frame.");
            icmp_frame_free(frame);
            break;
        }

        LOG_DBG("Enqueuing ICMP UART RX frame.");
        icmp_rx_enqueue(&frame, K_NO_WAIT);
        memset(icmp_rx_buf, 0, sizeof(icmp_rx_buf));

        break;

    default:
        break;
    }
}

int icmp_phy_uart_init(void)
{
    int ret = uart_callback_set(icmp_uart, icmp_uart_cb, NULL);
    if (ret) {
        LOG_ERR("ICMP UART init failed: %d", ret);
        return ret;
    }

    return uart_rx_enable(icmp_uart,
                          icmp_rx_buf,
                          sizeof(icmp_rx_buf),
                          ICMP_UART_RX_TIMEOUT);
}

static uint8_t frame_buf[ICMP_MAX_FRAME_SIZE] = {0};

int icmp_phy_uart_send(struct icmp_frame *frame)
{
    int frame_len = 0;
    memset(frame_buf, 0, ICMP_MAX_FRAME_SIZE);

    frame_len = icmp_frame_pack(frame, frame_buf, ICMP_MAX_FRAME_SIZE);
    if (frame_len < 1) {
        return frame_len;
    }

    return uart_tx(icmp_uart, frame_buf, frame_len, SYS_FOREVER_US);
}

const struct icmp_phy_api icmp_phy_uart = {
    .init = icmp_phy_uart_init,
    .send = icmp_phy_uart_send
};

const struct icmp_phy_api *icmp_get_selected_phy(void)
{
    return &icmp_phy_uart;
}
