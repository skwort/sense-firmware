#ifndef _LIB_CELLULAR_PACKET_H_
#define _LIB_CELLULAR_PACKET_H_

#include <zephyr/kernel.h>

struct cellular_packet {
    uint8_t buffer[CONFIG_CELLULAR_UPLINK_BUFFER_SIZE];
    size_t len;
};

int cellular_packet_alloc(struct cellular_packet **packet);

void cellular_packet_free(struct cellular_packet *packet);

uint32_t cellular_packet_allocated_count(void);

#endif /* _LIB_CELLULAR_PACKET_H */
