#ifndef _LIB_ICMP_FRAME_H_
#define _LIB_ICMP_FRAME_H_

#include <zephyr/sys/crc.h>
#include <lib/icmp.h>

#define ICMP_HEADER_SIZE 4
#define ICMP_CRC_SIZE    2
#define ICMP_FRAME_SIZE(payload_len) (ICMP_HEADER_SIZE + \
                                      ICMP_CRC_SIZE +    \
                                      payload_len)

#define ICMP_MIN_FRAME_SIZE ICMP_HEADER_SIZE + ICMP_CRC_SIZE

/**
 * @brief Packs an ICMP frame into a byte buffer.
 *
 * This function serialises an `icmp_frame` structure into a flat byte buffer,
 * appending the CRC16 at the end. The payload length in the frame must be
 * correctly set before calling this function.
 *
 * @param[in]  frame     Pointer to the populated ICMP frame structure.
 * @param[out] buf       Destination buffer to hold the packed frame.
 * @param[in]  buf_len   Length of the destination buffer.
 *
 * @return Number of bytes written to buf on success,
 *         -ENOBUFS if the buffer is too small,
 *         -EINVAL if the frame is invalid.
 */
int icmp_frame_pack(struct icmp_frame *frame, uint8_t *buf, size_t buf_len);

/**
 * @brief Unpacks a raw ICMP byte buffer into a frame structure.
 *
 * This function parses and validates a received ICMP frame. It checks the
 * CRC16 and extracts the header and payload fields. On failure, the frame
 * is not modified.
 *
 * @param[out] frame     Destination for the unpacked ICMP frame.
 * @param[in]  buf       Pointer to the raw input buffer.
 * @param[in]  buf_len   Length of the input buffer.
 *
 * @return 0 on success,
           -EINVAL if the buffer is malformed or the CRC check fails.
 */
int icmp_frame_unpack(struct icmp_frame *frame, uint8_t *buf, size_t buf_len);


static inline uint16_t icmp_crc16_calc(const uint8_t *data, size_t len)
{
    return crc16_ansi(data, len);
}

#endif /* _LIB_ICMP_FRAME_H_ */
