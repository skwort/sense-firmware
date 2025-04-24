#ifndef _LIB_ICMP_QUEUE_H_
#define _LIB_ICMP_QUEUE_H_

#include <zephyr/kernel.h>
#include <lib/icmp.h>

extern struct k_msgq icmp_tx_queue;
extern struct k_msgq icmp_rx_queue;

static inline int icmp_tx_enqueue(struct icmp_frame *frame,
                                  k_timeout_t timeout)
{
    return k_msgq_put(&icmp_tx_queue, &frame, timeout);
}

static inline int icmp_tx_dequeue(struct icmp_frame **frame,
                                  k_timeout_t timeout)
{
    return k_msgq_get(&icmp_tx_queue, (void **)frame, timeout);
}

static inline int icmp_rx_enqueue(struct icmp_frame *frame,
                                  k_timeout_t timeout)
{
    return k_msgq_put(&icmp_rx_queue, &frame, timeout);
}

static inline int icmp_rx_dequeue(struct icmp_frame **frame,
                                  k_timeout_t timeout)
{
    return k_msgq_get(&icmp_rx_queue, (void **)frame, timeout);
}

#endif /* _LIB_ICMP_QUEUE_H_ */
