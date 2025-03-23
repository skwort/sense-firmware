#include <zephyr/kernel.h>

#include "state.h"

K_MUTEX_DEFINE(system_state_mutex);

struct state system_state;
