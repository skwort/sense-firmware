#ifndef _DATAPOINT_QUEUE_H_
#define _DATAPOINT_QUEUE_H_

#include <zephyr/kernel.h>
#include <datapoint/czd_datapoint_types.h>

int datapoint_enqueue(struct datapoint *dp, k_timeout_t timeout);

int datapoint_dequeue(struct datapoint *dp, k_timeout_t timeout);

#endif /* _DATAPOINT_QUEUE_H_ */
