#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <lib/cellular.h>

#include "cellular_packet.h"
#include "cellular_backend.h"

K_SEM_DEFINE(rx_sem, 0, 1);

struct cellular_backend cellular_backend_mock;

void cellular_recv_cb(const uint8_t *payload, size_t payload_len)
{
    printk("Recv callback executed!\n");
    printk("Received: %s\n", payload);

    k_sem_give(&rx_sem);
}

int cellular_backend_mock_init_ok(void)
{
    return 0;
}

int cellular_backend_mock_init_fail(void)
{
    return 1;
}

const struct cellular_backend *cellular_get_selected_backend(void)
{
    return &cellular_backend_mock;
}

ZTEST(cellular_integration, test_cellular_integration)
{
    int err;
    int cellular_state;
    uint8_t req[] = "native_sim mock";

    /* Fail to start thread using bad receive callback */
    err = cellular_init(NULL);
    zassert_true(err != 0, "Cellular init didn't fail");

    cellular_state = cellular_state_get();
    zassert_true(cellular_state == CELLULAR_STATE_UNINITIALISED,
                 "Cellular state incorrect: %d", cellular_state);

    /* Start the cellular thread with a failing backend */
    cellular_backend_mock.init = cellular_backend_mock_init_fail;
    err = cellular_init(cellular_recv_cb);
    zassert_true(err == 0, "Cellular init failed: %d", err);

    /* Let the thread run briefly */
    k_sleep(K_MSEC(5));

    cellular_state = cellular_state_get();
    zassert_true(cellular_state == CELLULAR_STATE_BACKEND_ERROR,
                 "Cellular state incorrect: %d", cellular_state);

    /* Start the cellular thread */
    cellular_backend_mock.init = cellular_backend_mock_init_ok;
    err = cellular_init(cellular_recv_cb);
    zassert_true(err == 0, "Cellular init failed: %d", err);

    /* Let the thread run briefly */
    k_sleep(K_MSEC(5));

    cellular_state = cellular_state_get();
    zassert_true(cellular_state == CELLULAR_STATE_RUNNING,
                 "Cellular state incorrect: %d", cellular_state);

    /* Let the thread run briefly*/
    k_sleep(K_MSEC(5));

    err = cellular_init(cellular_recv_cb);
    zassert_true(err != 0, "Cellular init should have failed", err);

    /* Send a message */
    err = cellular_send_packet(req, sizeof(req));
    zassert_true(err == 0, "Cellular send packet failed: %d", err);

    k_sem_take(&rx_sem, K_FOREVER);

    /* Give the thread some time to free the packets */
    k_sleep(K_MSEC(5));

    /* Check that the packet memory blocks were free'd. */
    uint32_t num_used_packets = cellular_packet_allocated_count();
    zassert_true(num_used_packets == 0, "Packet(s) not free'd.");
}

ZTEST_SUITE(cellular_integration, NULL, NULL, NULL, NULL, NULL);
