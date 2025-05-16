#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "cellular_packet.h"
#include "cellular_queue.h"

ZTEST(cellular_queue, test_uplink_queue_basic)
{
    struct cellular_packet *in_pkt = k_malloc(sizeof(struct cellular_packet));
    struct cellular_packet *out_pkt = NULL;

    int ret = cellular_uplink_enqueue(&in_pkt, K_NO_WAIT);
    zassert_true(ret == 0, "Uplink enqueue failed.");

    ret = cellular_uplink_dequeue(&out_pkt, K_NO_WAIT);
    zassert_true(ret == 0, "Uplink dequeue failed.");

    zassert_equal_ptr(in_pkt, out_pkt, "In ptr is different to out ptr.");

    k_free(in_pkt);
}

ZTEST_SUITE(cellular_queue, NULL, NULL, NULL, NULL, NULL);
