#include <zephyr/kernel.h>
#include <lib/icmp.h>

#include "icmp_queue.h"

#define ICMP_QUEUE_MAX_ITEMS 8
#define ICMP_QUEUE_ALIGNMENT 4

K_MSGQ_DEFINE(icmp_tx_queue,
              sizeof(struct icmp_frame *),
              ICMP_QUEUE_MAX_ITEMS,
              ICMP_QUEUE_ALIGNMENT);

K_MSGQ_DEFINE(icmp_rx_queue,
              sizeof(struct icmp_frame *),
              ICMP_QUEUE_MAX_ITEMS,
              ICMP_QUEUE_ALIGNMENT);
