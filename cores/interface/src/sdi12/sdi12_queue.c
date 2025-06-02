#include <zephyr/kernel.h>

#include "sdi12_queue.h"

#define SDI12_QUEUE_ALIGNMENT 4
#define SDI12_CMD_QUEUE_SIZE 4
#define SDI12_RSP_QUEUE_SIZE 4

K_MSGQ_DEFINE(sdi12_cmd_q,
              SDI12_MSG_LEN,
              SDI12_CMD_QUEUE_SIZE,
              SDI12_QUEUE_ALIGNMENT);

K_MSGQ_DEFINE(sdi12_rsp_q,
              SDI12_MSG_LEN,
              SDI12_RSP_QUEUE_SIZE,
              SDI12_QUEUE_ALIGNMENT);

int sdi12_command_enqueue(const char msg[SDI12_MSG_LEN], k_timeout_t timeout)
{
    return k_msgq_put(&sdi12_cmd_q, (void *)msg, timeout);
}

int sdi12_command_dequeue(char msg[SDI12_MSG_LEN], k_timeout_t timeout)
{
    return k_msgq_get(&sdi12_cmd_q, msg, timeout);
}

int sdi12_response_enqueue(const char msg[SDI12_MSG_LEN], k_timeout_t timeout)
{
    return k_msgq_put(&sdi12_rsp_q, (void *)msg, timeout);
}

int sdi12_response_dequeue(char msg[SDI12_MSG_LEN], k_timeout_t timeout)
{
    return k_msgq_get(&sdi12_rsp_q, msg, timeout);
}
