#ifndef _CELLULAR_QUEUE_H_
#define _CELLULAR_QUEUE_H_

#include <zephyr/kernel.h>

#include "cellular_packet.h"

int cellular_uplink_enqueue(struct cellular_packet **packet,
                            k_timeout_t timeout);

int cellular_uplink_dequeue(struct cellular_packet **packet,
                            k_timeout_t timeout);

#endif /* _CELLULAR_QUEUE_H_ */
