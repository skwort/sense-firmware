#include <zephyr/kernel.h>

#include "cellular_packet.h"

K_MEM_SLAB_DEFINE(cellular_packet_slab,
                  sizeof(struct cellular_packet),
                  CONFIG_CELLULAR_UPLINK_QUEUE_MAX_ITEMS,
                  4);


int cellular_packet_alloc(struct cellular_packet **packet)
{
    return k_mem_slab_alloc(&cellular_packet_slab, (void **)packet, K_NO_WAIT);
}

void cellular_packet_free(struct cellular_packet *packet)
{
    return k_mem_slab_free(&cellular_packet_slab, (void *)packet);
}

uint32_t cellular_packet_allocated_count(void)
{
    return k_mem_slab_num_used_get(&cellular_packet_slab);
}
