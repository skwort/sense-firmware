#include <zephyr/kernel.h>
#include <string.h>
#include <lib/icmp.h>
#include "icmp_frame.h"

K_MEM_SLAB_DEFINE(icmp_slab,                      \
                  sizeof(struct icmp_frame),      \
                  CONFIG_ICMP_MAX_MEM_SLAB_FRAMES, \
                  4);

int icmp_frame_pack(struct icmp_frame *frame, uint8_t *buf, size_t buf_len)
{
    /* Ensure frame and buf are non-null */
    if (!frame || !buf) {
        return -EINVAL;
    }

    /* Validate message type */
    if (frame->type >= ICMP_TYPE_INVALID) {
        return -EINVAL;
    }

    /* Validate payload length */
    if (frame->length > ICMP_MAX_PAYLOAD_SIZE) {
        return -EINVAL;
    }

    /* Validate buffer size */
    size_t frame_len = ICMP_FRAME_SIZE(frame->length);
    if (frame_len > buf_len) {
        return -ENOBUFS;
    }

    /* Pack the frame into the buffer */
    buf[0] = frame->type;
    buf[1] = frame->msg_id;
    buf[2] = frame->target;
    buf[3] = frame->length;
    memcpy(&buf[4], frame->payload, frame->length);

    /* Compute and store the CRC16 */
    uint16_t crc = icmp_crc16_calc(buf, 4 + frame->length);
    buf[4 + frame->length] = crc >> 8;
    buf[5 + frame->length] = crc & 0xFF;

    return frame_len;
}

int icmp_frame_unpack(struct icmp_frame *frame, uint8_t *buf, size_t buf_len)
{
    /* Ensure frame and buf are non-null */
    if (!frame || !buf) {
        return -EINVAL;
    }

    if (buf_len < ICMP_MIN_FRAME_SIZE) {
        return -EINVAL;
    }

    /* Unpack the buffer into a temporary frame */
    struct icmp_frame temp_frame = {0};
    temp_frame.type = buf[0];
    temp_frame.msg_id = buf[1];
    temp_frame.target = buf[2];
    temp_frame.length = buf[3];

    /* Validate message type */
    if (temp_frame.type >= ICMP_TYPE_INVALID) {
        return -EINVAL;
    }

    /* Validate payload length */
    if (temp_frame.length > ICMP_MAX_PAYLOAD_SIZE) {
        return -EINVAL;
    }

    /* Compute and check the CRC16 */
    uint16_t sent_crc = (buf[4 + temp_frame.length] << 8) |
                     buf[5 + temp_frame.length];

    uint16_t calc_crc = icmp_crc16_calc(buf, 4 + temp_frame.length);
    if (calc_crc != sent_crc) {
        return -EINVAL;
    }

    memcpy(temp_frame.payload, &buf[4], temp_frame.length);

    memcpy(frame, &temp_frame, sizeof(temp_frame));

    return 0;
}
