#ifndef CONTROL_CORE_STATE_H_
#define CONTROL_CORE_STATE_H_

#include <zephyr/kernel.h>

enum state_codes {
    OK = 0,
    INIT_FAILED,
    DATA_ERROR,
    SLEEPING,
};

struct state {
#ifdef CONFIG_HEARTBEAT
    uint8_t heartbeat_state;
#endif
};

/* Global state variable */
extern struct state system_state;

/* Global state lock */
extern struct k_mutex system_state_mutex;

static inline int state_lock(k_timeout_t timeout)
{
    return k_mutex_lock(&system_state_mutex, timeout);
}

static inline int state_unlock(void)
{
    return k_mutex_unlock(&system_state_mutex);
}

static inline struct state *state_get(void)
{
    return &system_state;
}

#endif /* CONTROL_CORE_STATE_H_ */
