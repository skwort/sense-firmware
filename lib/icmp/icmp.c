#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lib/icmp.h>

#include "icmp_queue.h"
#include "icmp_frame.h"

LOG_MODULE_REGISTER(icmp, CONFIG_ICMP_LOG_LEVEL);

icmp_callback_t rx_dispatch_cb[CONFIG_ICMP_MAX_TARGETS];


int icmp_register_target(uint8_t target_id, icmp_callback_t callback)
{
    if (target_id < 0 || target_id >= CONFIG_ICMP_MAX_TARGETS) {
        return -EINVAL;
    }

    rx_dispatch_cb[target_id] = callback;
    return 0;
}

static int icmp_send_frame(uint8_t type,
                           uint8_t msg_id,
                           uint8_t target_id,
                           const uint8_t *payload,
                           size_t payload_len)
{
    struct icmp_frame *frame = NULL;
    int ret = icmp_frame_alloc(&frame);
    if (ret != 0) {
        return ret;
    }

    frame->type = type;
    frame->msg_id = msg_id;
    frame->target = target_id;
    frame->length = payload_len;
    memcpy(frame->payload, payload, payload_len);

    return icmp_tx_enqueue(&frame, K_NO_WAIT);
}

int icmp_command(uint8_t target_id,
                 const uint8_t *payload,
                 size_t payload_len,
                 icmp_response_cb_t cb,
                 void *user_data)
{
    if (payload_len > ICMP_MAX_PAYLOAD_SIZE) {
        return -EINVAL;
    }

    /* Generate msg_id */
    uint8_t msg_id = 0;

    return icmp_send_frame(ICMP_TYPE_COMMAND,
                           msg_id,
                           target_id,
                           payload,
                           payload_len);
}

int icmp_respond(uint8_t target_id,
                 uint8_t msg_id,
                 const uint8_t *payload,
                 size_t payload_len)
{
    if (payload_len > ICMP_MAX_PAYLOAD_SIZE) {
        return -EINVAL;
    }

    return icmp_send_frame(ICMP_TYPE_RESPONSE,
                           msg_id,
                           target_id,
                           payload,
                           payload_len);
}

int icmp_notify(uint8_t target_id,
                const uint8_t *payload,
                size_t payload_len)
{
    if (payload_len > ICMP_MAX_PAYLOAD_SIZE) {
        return -EINVAL;
    }

    /* NOTE: We use 255 as a dummy message id for NOTIFY frames. */
    return icmp_send_frame(ICMP_TYPE_NOTIFY,
                           255,
                           target_id,
                           payload,
                           payload_len);
}
