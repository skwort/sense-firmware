#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <lib/icmp.h>

#include "icmp_queue.h"
#include "icmp_frame.h"
#include "icmp_phy.h"

K_SEM_DEFINE(rx_basic_sem, 0, 1);
K_SEM_DEFINE(rx_response_sem, 0, 1);
K_SEM_DEFINE(tx_sem, 0, 1);

#define PHY_SEND_NORMAL 0
#define PHY_SEND_DELAY  1
static volatile uint8_t phy_send_mode = PHY_SEND_NORMAL;

static volatile uint8_t msg_id = 0;

void rx_basic_callback(const uint8_t *payload, size_t payload_len)
{
    ARG_UNUSED(payload);
    ARG_UNUSED(payload_len);

    printk("Basic callback executed!\n");

    k_sem_give(&rx_basic_sem);
}

void rx_response_callback(const uint8_t *payload,
                          size_t payload_len,
                          void *user_data)
{
    ARG_UNUSED(payload);
    ARG_UNUSED(payload_len);
    ARG_UNUSED(user_data);

    printk("Response callback executed!\n");

    k_sem_give(&rx_response_sem);
}

int icmp_phy_mock_init(void)
{
    return 0;
}

int icmp_phy_mock_send(struct icmp_frame *frame)
{
    struct icmp_frame *rx_frame;
    int ret = icmp_frame_alloc(&rx_frame);
    zassert_true(ret == 0, "icmp_frame_alloc failed: %d", ret);

    memcpy(rx_frame, frame, sizeof(struct icmp_frame));

    /* Store the msg_id */
    msg_id = frame->msg_id;

    if (phy_send_mode == PHY_SEND_DELAY) {
        k_sleep(K_MSEC(CONFIG_ICMP_MAX_INFLIGHT_MSG_AGE + 500));
    }

    ret = icmp_rx_enqueue(&rx_frame, K_NO_WAIT);
    zassert_true(ret == 0, "rx enqueue failed: %d", ret);

    k_sem_give(&tx_sem);

    return 0;
}

/* ICMP PHY Mock API */
const struct icmp_phy_api icmp_phy_mock = {
    .init = icmp_phy_mock_init,
    .send = icmp_phy_mock_send
};

const struct icmp_phy_api *icmp_get_selected_phy(void)
{
    return &icmp_phy_mock;
}

void test_tx_rx_notify(void)
{
    uint8_t buf[5] = {'x'};

    /* Send a notification */
    icmp_notify(0, buf, 5);

    /* Wait for tx confirmation */
    int ret = k_sem_take(&tx_sem, K_FOREVER);
    zassert_true(ret == 0, "Unexpected return %d");

    /* Wait for rx confirmation via the callback */
    ret = k_sem_take(&rx_basic_sem, K_FOREVER);
    zassert_true(ret == 0, "Unexpected return %d");

    /* Sleep */
    k_sleep(K_MSEC(5));

    /* Check that the memory blocks containing the frames has been free'd. */
    uint32_t num_used_slabs = k_mem_slab_num_used_get(&icmp_slab);
    zassert_true(num_used_slabs == 0, "Frame not free'd.");

}

void test_tx_rx_command(void)
{
    uint8_t buf[5] = {'x'};

    /* Send a command, registering a response callback */
    icmp_command(0, buf, 5, rx_response_callback, NULL);

    /* Wait for tx confirmation */
    int ret = k_sem_take(&tx_sem, K_FOREVER);
    zassert_true(ret == 0, "Unexpected return %d");

    /* Wait for rx confirmation via the callback */
    ret = k_sem_take(&rx_basic_sem, K_FOREVER);
    zassert_true(ret == 0, "Unexpected return %d");

    /* Sleep */
    k_sleep(K_MSEC(5));

    /* Check that the memory blocks containing the frames has been free'd. */
    uint32_t num_used_slabs = k_mem_slab_num_used_get(&icmp_slab);
    zassert_true(num_used_slabs == 0, "Frame not free'd.");
}

void test_tx_rx_response(void)
{
    uint8_t buf[5] = {'x'};

    /* Send a response, using the message ID received in test_tx_rx_command */
    icmp_respond(0, msg_id, buf, 5);

    /* Wait for tx confirmation */
    int ret = k_sem_take(&tx_sem, K_FOREVER);
    zassert_true(ret == 0, "Unexpected return %d");

    /* Wait for rx confirmation via the callback */
    ret = k_sem_take(&rx_response_sem, K_FOREVER);
    zassert_true(ret == 0, "Unexpected return %d");

    /* Sleep */
    k_sleep(K_MSEC(5));

    /* Check that the memory blocks containing the frames has been free'd. */
    uint32_t num_used_slabs = k_mem_slab_num_used_get(&icmp_slab);
    zassert_true(num_used_slabs == 0, "Frame not free'd.");
}

void test_tx_command_dropped(void)
{
    uint8_t buf[5] = {'x'};

    /* Send a command, registering a response callback */
    icmp_command(0, buf, 5, rx_response_callback, NULL);

    /* Wait for tx confirmation */
    int ret = k_sem_take(&tx_sem, K_FOREVER);
    zassert_true(ret == 0, "Unexpected return %d");

    /* Wait for rx confirmation */
    ret = k_sem_take(&rx_basic_sem, K_FOREVER);
    zassert_true(ret == 0, "Unexpected return %d");

    /* Enable a delay in the PHY send function */
    phy_send_mode = PHY_SEND_DELAY;

    /* Now send the response */
    icmp_respond(0, msg_id, buf, 5);

    /* Wait for tx confirmation */
    ret = k_sem_take(&tx_sem, K_FOREVER);
    zassert_true(ret == 0, "Unexpected return %d");

    /* Wait for rx confirmation via the callback. Note that because we enabled
     * a delay in the PHY send, we should expect the dispatch table entry to
     * be aged out or dropped. This means that the standard callback should be
     * called, rather than the response callback. */
    ret = k_sem_take(&rx_basic_sem, K_FOREVER);
    zassert_true(ret == 0, "Unexpected return %d");

    /* Sleep */
    k_sleep(K_MSEC(5));

    /* Check that the memory blocks containing the frames has been free'd. */
    uint32_t num_used_slabs = k_mem_slab_num_used_get(&icmp_slab);
    zassert_true(num_used_slabs == 0, "Frame not free'd.");

}

ZTEST(icmp_integration, test_icmp_integration)
{
    /* Register rx_callback with target_id 0 */
    int ret = icmp_register_target(0,
                                   (icmp_callback_t)rx_basic_callback);
    zassert_true(ret == 0, "register target failed: %d", ret);

    /* Start the ICMP thread */
    icmp_init();

    /* Let the thread run briefly */
    k_sleep(K_MSEC(5));

    test_tx_rx_notify();

    /* NOTE: the command and response tests are coupled. The msg_id from the
     * command test is stored and used in the response test. This allows us to
     * validate response callback registration . */
    test_tx_rx_command();
    test_tx_rx_response();

    /* Test aging of in-flight messages */
    test_tx_command_dropped();
}

ZTEST_SUITE(icmp_integration, NULL, NULL, NULL, NULL, NULL);
