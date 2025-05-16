#include <zephyr/kernel.h>

#include "cellular_packet.h"
#include "cellular_queue.h"

#define CELLULAR_QUEUE_ALIGNMENT 4

K_MSGQ_DEFINE(cellular_uplink_queue,
              sizeof(struct cellular_packet *),
              CONFIG_CELLULAR_UPLINK_QUEUE_MAX_ITEMS,
              CELLULAR_QUEUE_ALIGNMENT);

int cellular_uplink_enqueue(struct cellular_packet **packet,
                            k_timeout_t timeout)
{
    return k_msgq_put(&cellular_uplink_queue, (void **)packet, timeout);
}

int cellular_uplink_dequeue(struct cellular_packet **packet,
                            k_timeout_t timeout)
{
    return k_msgq_get(&cellular_uplink_queue, (void **)packet, timeout);
}
