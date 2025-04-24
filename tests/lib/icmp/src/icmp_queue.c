#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <lib/icmp.h>
#include "icmp_queue.h"

/* Fixture for generating a valid frame */
static void valid_frame(struct icmp_frame * frame)
{
	frame->type = ICMP_TYPE_COMMAND,
	frame->msg_id = 0x01;
	frame->target = 0x01;
	frame->length = 5;
    frame->payload[0] = 'H';
    frame->payload[1] = 'e';
    frame->payload[2] = 'l';
    frame->payload[3] = 'l';
    frame->payload[4] = 'o';
}

ZTEST(icmp_queue, test_tx_queue_basic)
{
    struct icmp_frame *in_frame = k_malloc(sizeof(struct icmp_frame));
    struct icmp_frame *out_frame = NULL;

    valid_frame(in_frame);

    int ret = icmp_tx_enqueue(in_frame, K_NO_WAIT);
    zassert_true(ret == 0, "TX enqueue failed.");

    ret = icmp_tx_dequeue(&out_frame, K_NO_WAIT);
    zassert_true(ret == 0, "TX dequeue failed.");

    zassert_equal_ptr(in_frame, out_frame, "In ptr is different to out ptr.");

    k_free(in_frame);
}

ZTEST(icmp_queue, test_rx_queue_basic)
{
    struct icmp_frame *in_frame = k_malloc(sizeof(struct icmp_frame));
    struct icmp_frame *out_frame = NULL;

    valid_frame(in_frame);

    int ret = icmp_rx_enqueue(in_frame, K_NO_WAIT);
    zassert_true(ret == 0, "TX enqueue failed.");

    ret = icmp_rx_dequeue(&out_frame, K_NO_WAIT);
    zassert_true(ret == 0, "TX dequeue failed.");

    zassert_equal_ptr(in_frame, out_frame, "In ptr is different to out ptr.");

    k_free(in_frame);
}

ZTEST_SUITE(icmp_queue, NULL, NULL, NULL, NULL, NULL);
