#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <lib/icmp.h>
#include "icmp_queue.h"

/* Fixture for generating a valid frame */
static struct icmp_frame valid_frame(void)
{
	struct icmp_frame frame = {
		.type = ICMP_TYPE_COMMAND,
		.msg_id = 0x01,
		.target = 0x01,
		.length = 5,
		.payload = { 'H', 'e', 'l', 'l', 'o' }
	};
	return frame;
}

ZTEST(icmp_queue, test_tx_queue_basic)
{
    struct icmp_frame in_frame = valid_frame();
    struct icmp_frame out_frame;

    int ret = icmp_tx_enqueue(&in_frame, K_NO_WAIT);
    zassert_true(ret == 0, "TX enqueue failed.");

    ret = icmp_tx_dequeue(&out_frame, K_NO_WAIT);
    zassert_true(ret == 0, "TX dequeue failed.");

    zassert_mem_equal(&in_frame, &out_frame, sizeof(struct icmp_frame));
}

ZTEST(icmp_queue, test_rx_queue_basic)
{
    struct icmp_frame in_frame = valid_frame();
    struct icmp_frame out_frame;

    int ret = icmp_rx_enqueue(&in_frame, K_NO_WAIT);
    zassert_true(ret == 0, "TX enqueue failed.");

    ret = icmp_rx_dequeue(&out_frame, K_NO_WAIT);
    zassert_true(ret == 0, "TX dequeue failed.");

    zassert_mem_equal(&in_frame, &out_frame, sizeof(struct icmp_frame));
}

ZTEST_SUITE(icmp_queue, NULL, NULL, NULL, NULL, NULL);
