#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <lib/icmp.h>

#include "icmp_queue.h"
#include "icmp_frame.h"
#include "icmp_phy.h"

K_SEM_DEFINE(rx_sem, 0, 1);
K_SEM_DEFINE(tx_sem, 0, 1);

void rx_callback(const uint8_t *payload, size_t payload_len)
{
    ARG_UNUSED(payload);
    ARG_UNUSED(payload_len);

    printk("Callback executed!\n");

    k_sem_give(&rx_sem);
}

int icmp_phy_mock_init(void)
{
    return 0;
}

int icmp_phy_mock_send(struct icmp_frame *frame)
{
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

void test_rx_path(void)
{
    /* Allocate and populate the frame */
    struct icmp_frame *frame;
    icmp_frame_alloc(&frame);
    frame->type = ICMP_TYPE_COMMAND,
	frame->msg_id = 0x00;
	frame->target = 0x00;
	frame->length = 5;

    /* Enqueue the packet */
    int ret = icmp_rx_enqueue(&frame, K_NO_WAIT);
    zassert_true(ret == 0, "rx enqueue failed: %d", ret);

    /* Wait for the semaphore. The callback should be triggered by the work
     * handler, which should give the semaphore. */
    ret = k_sem_take(&rx_sem, K_FOREVER);
    zassert_true(ret == 0, "Unexpected return %d");

    /* Check that the memory block containing the frame has been free'd. */
    uint32_t num_used_slabs = k_mem_slab_num_used_get(&icmp_slab);
    zassert_true(num_used_slabs == 0, "Frame not free'd.");

    /* Now enqueue a packet for an unregistered target. */
    icmp_frame_alloc(&frame);
    frame->type = ICMP_TYPE_COMMAND,
	frame->msg_id = 0x00;
	frame->target = 0x05;
	frame->length = 5;

    ret = icmp_rx_enqueue(&frame, K_NO_WAIT);
    zassert_true(ret == 0, "rx enqueue failed: %d", ret);

    /* The semaphore will never be given. */
    ret = k_sem_take(&rx_sem, K_MSEC(50));
    zassert_true(ret == -EAGAIN, "Unexpected return %d");

    /* Check that the memory block containing the frame has been free'd. */
    num_used_slabs = k_mem_slab_num_used_get(&icmp_slab);
    zassert_true(num_used_slabs == 0, "Frame not free'd.");

}

void test_tx_path(void)
{
    uint8_t buf[5] = {'x'};

    /* Send a notification */
    icmp_notify(0, buf, 5);

    /* Wait for tx confirmation */
    int ret = k_sem_take(&tx_sem, K_FOREVER);
    zassert_true(ret == 0, "Unexpected return %d");

    /* Sleep */
    k_sleep(K_MSEC(5));

    /* Check that the memory block containing the frame has been free'd. */
    uint32_t num_used_slabs = k_mem_slab_num_used_get(&icmp_slab);
    zassert_true(num_used_slabs == 0, "Frame not free'd.");

}

ZTEST(icmp_integration, test_icmp_integration)
{
    /* Register rx_callback with target_id 0 */
    int ret = icmp_register_target(0,
                                   (icmp_callback_t)rx_callback);
    zassert_true(ret == 0, "register target failed: %d", ret);

    /* Start the ICMP thread */
    icmp_init();

    /* Let the thread run briefly */
    k_sleep(K_MSEC(5));

    test_tx_path();
    test_rx_path();
}

ZTEST_SUITE(icmp_integration, NULL, NULL, NULL, NULL, NULL);
