#ifndef LIB_ICMP_H_
#define LIB_ICMP_H_

#include <zephyr/kernel.h>

#ifdef CONFIG_ICMP

#define ICMP_MAX_PAYLOAD_SIZE CONFIG_ICMP_MAX_PAYLOAD_SIZE

enum icmp_type {
    ICMP_TYPE_COMMAND   = 0x01,
    ICMP_TYPE_RESPONSE  = 0x02,
    ICMP_TYPE_NOTIFY    = 0x03,
    ICMP_TYPE_HEARTBEAT = 0x04,
    ICMP_TYPE_INVALID
};

struct icmp_frame {
    uint8_t  type;
    uint8_t  msg_id;
    uint8_t  target;
    uint8_t  length;
    uint8_t  payload[ICMP_MAX_PAYLOAD_SIZE];
};

#endif /* CONFIG_ICMP */

#endif /* LIB_ICMP_H_ */
