#ifndef LIB_ICMP_H_
#define LIB_ICMP_H_

#include <zephyr/kernel.h>

#ifdef CONFIG_ICMP

#define ICMP_MAX_PAYLOAD_SIZE CONFIG_ICMP_MAX_PAYLOAD_SIZE

enum icmp_type {
    ICMP_TYPE_COMMAND   = 0x01,
    ICMP_TYPE_RESPONSE  = 0x02,
    ICMP_TYPE_NOTIFY    = 0x03,
    ICMP_TYPE_HEARTBEAT = 0x04,
    ICMP_TYPE_INVALID
};

struct icmp_frame {
    uint8_t  type;
    uint8_t  msg_id;
    uint8_t  target;
    uint8_t  length;
    uint8_t  payload[ICMP_MAX_PAYLOAD_SIZE];
};

/* Callback type for handling received frames */
typedef void (*icmp_callback_t)(const uint8_t *payload, size_t payload_len);

/* Callback type for command response */
typedef void (*icmp_response_cb_t)(int result,
                                   const uint8_t *payload,
                                   size_t payload_len,
                                   void *user_data);

/**
 * Initialise the ICMP server.
 *
 * @return 0 on success, negative value on error
 */
int icmp_init(void);

/**
 * Register a target with the ICMP server.
 *
 * @param[in] target_id  Target address (unique per MCU)
 * @param[in] callback   Callback invoked on reception of message for this
                         target
 * @return               0 on success, negative value on error
 */
int icmp_register_target(uint8_t target_id, icmp_callback_t callback);

/**
 * Send a command to a remote target. Expects a response.
 *
 * @param[in] target_id   Remote target ID
 * @param[in] payload     Pointer to payload buffer
 * @param[in] payload_len Length of payload
 * @param[in] cb          Optional response callback (can be NULL)
 * @param[in] user_data   Context pointer passed to response callback
 * @return                0 on success, negative value on error
 */
int icmp_command(uint8_t target_id,
                 const uint8_t *payload,
                 size_t payload_len,
                 icmp_response_cb_t cb,
                 void *user_data);

/**
 * Send a response to a received command.
 *
 * @param[in] target_id   Target to respond to
 * @param[in] msg_id      Message ID of the original command
 * @param[in] payload     Pointer to payload buffer
 * @param[in] payload_len Length of payload
 * @return                0 on success, negative value on error
 */
int icmp_respond(uint8_t target_id,
                 uint8_t msg_id,
                 const uint8_t *payload,
                 size_t payload_len);

/**
 * Send a notify message to a remote target. No response expected.
 *
 * @param[in] target_id   Remote target ID
 * @param[in] payload     Pointer to payload buffer
 * @param[in] payload_len Length of payload
 * @return                0 on success, negative value on error
 */
int icmp_notify(uint8_t target_id,
                const uint8_t *payload,
                size_t payload_len);

#endif /* CONFIG_ICMP */

#endif /* LIB_ICMP_H_ */
