#ifndef SDI12_QUEUE_H_
#define SDI12_QUEUE_H_

#include <zephyr/kernel.h>

#define SDI12_MSG_LEN 80

int sdi12_command_enqueue(const char msg[SDI12_MSG_LEN], k_timeout_t timeout);

int sdi12_command_dequeue(char msg[SDI12_MSG_LEN], k_timeout_t timeout);

int sdi12_response_enqueue(const char msg[SDI12_MSG_LEN], k_timeout_t timeout);

int sdi12_response_dequeue(char msg[SDI12_MSG_LEN], k_timeout_t timeout);

#endif /* SDI12_QUEUE_H_ */
