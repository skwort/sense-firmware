#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "cellular_packet.h"

ZTEST(cellular_packet, test_packet_alloc_and_free)
{
    struct cellular_packet *packet;
    int ret = cellular_packet_alloc(&packet);
    zassert_true(ret == 0, "Packet alloc failed");
    cellular_packet_free(packet);
}

ZTEST_SUITE(cellular_packet, NULL, NULL, NULL, NULL, NULL);
