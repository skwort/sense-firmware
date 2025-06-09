#ifndef REMOTE_COMMANDS_H_
#define REMOTE_COMMANDS_H_

#include <zephyr/kernel.h>

int remote_commands_init(void);

void remote_commands_set_poll_interval(int32_t interval_ms);

void remote_commands_handle_packet(const uint8_t *buf, size_t len);

#endif /* REMOTE_COMMANDS_H_ */
