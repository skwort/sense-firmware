#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <lib/icmp.h>

#include "icmp_queue.h"
#include "icmp_frame.h"

ZTEST(icmp, test_icmp_command_ok)
{
    /* Send the command */
    uint8_t payload[] = { 'H', 'e', 'l', 'l', 'o'};
    int ret = icmp_command(0x01, payload, 5, NULL, NULL);
    zassert_true(ret == 0, "Failed to issue ICMP COMMAND: %d", ret);

    /* Send the command */
    struct icmp_frame *frame = NULL;
    ret = icmp_tx_dequeue(&frame, K_NO_WAIT);
    zassert_true(ret == 0,
                 "Failed to extract item from icmp tx_queue: %d", ret);

	zassert_equal(frame->type, ICMP_TYPE_COMMAND);
	zassert_equal(frame->target, 0x01);
	zassert_equal(frame->length, 5);
    zassert_mem_equal(frame->payload, payload, 5);

    icmp_frame_free(frame);
}

ZTEST(icmp, test_icmp_response_ok)
{
    /* Send the command */
    uint8_t payload[] = { 'H', 'e', 'l', 'l', 'o'};
    int ret = icmp_respond(0x01, 0x02, payload, 5);
    zassert_true(ret == 0, "Failed to issue ICMP RESPONSE: %d", ret);

    /* Send the command */
    struct icmp_frame *frame = NULL;
    ret = icmp_tx_dequeue(&frame, K_NO_WAIT);
    zassert_true(ret == 0,
                 "Failed to extract item from icmp tx_queue: %d", ret);

	zassert_equal(frame->type, ICMP_TYPE_RESPONSE);
	zassert_equal(frame->msg_id, 0x02);
	zassert_equal(frame->target, 0x01);
	zassert_equal(frame->length, 5);
    zassert_mem_equal(frame->payload, payload, 5);

    icmp_frame_free(frame);
}

ZTEST(icmp, test_icmp_notify_ok)
{
    /* Send the command */
    uint8_t payload[] = { 'H', 'e', 'l', 'l', 'o'};
    int ret = icmp_notify(0x01, payload, 5);
    zassert_true(ret == 0, "Failed to issue ICMP RESPONSE: %d", ret);

    /* Send the command */
    struct icmp_frame *frame = NULL;
    ret = icmp_tx_dequeue(&frame, K_NO_WAIT);
    zassert_true(ret == 0,
                 "Failed to extract item from icmp tx_queue: %d", ret);

	zassert_equal(frame->type, ICMP_TYPE_NOTIFY);
	zassert_equal(frame->target, 0x01);
	zassert_equal(frame->length, 5);
    zassert_mem_equal(frame->payload, payload, 5);

    icmp_frame_free(frame);
}

ZTEST(icmp, test_icmp_command_invalid_payload)
{
    uint8_t payload[ICMP_MAX_PAYLOAD_SIZE + 1] = {'x'};
    int ret = icmp_command(0x01, payload, sizeof(payload), NULL, NULL);
    zassert_true(ret == -EINVAL, "Unexpected return: %d", ret);
}

ZTEST(icmp, test_icmp_respond_invalid_payload)
{
    uint8_t payload[ICMP_MAX_PAYLOAD_SIZE + 1] = {'x'};
    int ret = icmp_respond(0x01, 0x02, payload, sizeof(payload));
    zassert_true(ret == -EINVAL, "Unexpected return: %d", ret);
}

ZTEST(icmp, test_icmp_notify_invalid_payload)
{
    uint8_t payload[ICMP_MAX_PAYLOAD_SIZE + 1] = {'x'};
    int ret = icmp_notify(0x01, payload, 65);
    zassert_true(ret == -EINVAL, "Unexpected return: %d", ret);
}

ZTEST(icmp, test_icmp_send_frame_fail)
{
    uint8_t payload[] = { 'H', 'e', 'l', 'l', 'o'};
    int ret;

    /* Exhaust all the slabs */
    for (int i = 0; i < CONFIG_ICMP_MAX_MEM_SLAB_FRAMES; i++) {
        ret = icmp_notify(0x01, payload, 5);
        zassert_true(ret == 0, "Unexpected return: %d", ret);
    }

    /* Try to allocate one more */
    ret = icmp_notify(0x01, payload, 5);
    zassert_true(ret == -ENOMEM, "Unexpected return: %d", ret);

    /* Clean-up */
    for (int i = 0; i < CONFIG_ICMP_MAX_MEM_SLAB_FRAMES; i++) {
        struct icmp_frame *frame = NULL;
        ret = icmp_tx_dequeue(&frame, K_NO_WAIT);
        zassert_true(ret == 0,
                    "Failed to extract item from icmp tx_queue: %d", ret);
        icmp_frame_free(frame);
    }
}

void rx_callback(const uint8_t *payload, size_t payload_len)
{
    ARG_UNUSED(payload);
    ARG_UNUSED(payload_len);
}

ZTEST(icmp, test_icmp_register_target_ok)
{
    int ret = icmp_register_target(0x01, (icmp_callback_t)rx_callback);
    zassert_true(ret == 0, "Unexpected return: %d", ret);
}

ZTEST(icmp, test_icmp_register_target_fail_target)
{
    int ret = icmp_register_target(CONFIG_ICMP_MAX_TARGETS,
                                   (icmp_callback_t)rx_callback);
    zassert_true(ret == -EINVAL, "Unexpected return: %d", ret);
}

ZTEST(icmp, test_icmp_register_target_fail_cb)
{
    int ret = icmp_register_target(0x01, NULL);
    zassert_true(ret == -EINVAL, "Unexpected return: %d", ret);
}

ZTEST_SUITE(icmp, NULL, NULL, NULL, NULL, NULL);
