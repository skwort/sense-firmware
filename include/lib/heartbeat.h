#ifndef LIB_HEARTBEAT_H_
#define LIB_HEARTBEAT_H_

#include <zephyr/kernel.h>

int heartbeat_init(k_timeout_t duration);

#endif /* LIB_HEARTBEAT_H_ */