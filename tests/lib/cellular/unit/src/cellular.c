#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <lib/cellular.h>

#include "cellular_queue.h"
#include "cellular_packet.h"

ZTEST(cellular, test_send_packet_bad_args)
{
    int err = 0;
    uint8_t buffer[] = "message";

    /* Null buffer fails */
    err = cellular_send_packet(NULL, 30);
    zassert_true(err != 0, "Send packet didn't fail");

    /* Length 0 buffer fails */
    err = cellular_send_packet(buffer, 0);
    zassert_true(err != 0, "Send packet didn't fail");

    /* Over-sized buffer fails */
    err = cellular_send_packet(buffer, CONFIG_CELLULAR_UPLINK_BUFFER_SIZE + 1);
    zassert_true(err != 0, "Send packet didn't fail");
}

ZTEST(cellular, test_send_packet_no_free_packets)
{
    int err = 0;
    uint8_t buffer[] = "message";

    /* Exhaust the packet allocation */
    for (int i = 0; i < CONFIG_CELLULAR_UPLINK_QUEUE_MAX_ITEMS; i++) {
        err = cellular_send_packet(buffer, 7);
        zassert_true(err == 0, "Packet failed to send: %d", err);
    }

    /* Fail to send another packet due to allocation exhaustion */
    err = cellular_send_packet(buffer, sizeof(buffer));
    zassert_true(err != 0, "Packet send should have failed.");

    /* Clean-up */
    struct cellular_packet *packet = NULL;
    for (int i = 0; i < CONFIG_CELLULAR_UPLINK_QUEUE_MAX_ITEMS; i++) {
        err = cellular_uplink_dequeue(&packet, K_FOREVER);
        zassert_true(err == 0, "Failed to dequeue packet: %d", err);
        cellular_packet_free(packet);
        packet = NULL;
    }

    uint32_t num_used_packets = cellular_packet_allocated_count();
    zassert_true(num_used_packets == 0, "Packet(s) not free'd.");
}

ZTEST_SUITE(cellular, NULL, NULL, NULL, NULL, NULL);
