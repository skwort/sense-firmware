#include <zephyr/kernel.h>

#include <datapoint_queue.h>
#include <datapoint/czd_datapoint_types.h>

#define DATAPOINT_QUEUE_ALIGNMENT 4
#define DATAPOINT_QUEUE_MAX_ITEMS 10

K_MSGQ_DEFINE(datapoint_queue,
              sizeof(struct datapoint),
              DATAPOINT_QUEUE_MAX_ITEMS,
              DATAPOINT_QUEUE_ALIGNMENT);

int datapoint_enqueue(struct datapoint *dp, k_timeout_t timeout)
{
    return k_msgq_put(&datapoint_queue, dp, timeout);
}

int datapoint_dequeue(struct datapoint *dp, k_timeout_t timeout)
{
    return k_msgq_get(&datapoint_queue, dp, timeout);
}
