#ifndef LIB_CELLULAR_H_
#define LIB_CELLULAR_H_

#ifdef CONFIG_CELLULAR

/**
 * @brief Cellular module runtime state.
 *
 * Represents the state of the cellular subsystem. Can be used to check if
 * the module is operational or in an error state.
 *
 * Each of the *_ERROR states indicate that the cellular module has stopped.
 */
 typedef enum cellular_state {
    CELLULAR_STATE_UNINITIALISED = 0,   /**< Module not initialised */
    CELLULAR_STATE_STARTING,            /**< Module is starting */
    CELLULAR_STATE_RUNNING,             /**< Module is running normally */
    CELLULAR_STATE_BACKEND_ERROR,       /**< Backend error */
    CELLULAR_STATE_SOCKET_ERROR,        /**< Socket error */
    CELLULAR_STATE_REMOTE_SERVER_ERROR, /**< Remote server error */
    CELLULAR_STATE_UNDEFINED,           /**< Unknown state */
} cellular_state_t;

/* Callback type for handling received packets */
typedef void (*cellular_recv_cb_t)(const uint8_t *packet, size_t packet_len);

/**
 * @brief Initialise the cellular module.
 *
 * Initialises the cellular module, enabling it to send and receive packets
 * to and from a remote server. Incoming packets are delivered to the provided
 * callback function.
 *
 * @param[in] cb  Callback function to handle received packets.
 *
 * @return 0 on success, negative value on error.
 */
int cellular_init(cellular_recv_cb_t cb);

/**
 * @brief Send a packet through the cellular interface.
 *
 * Sends a packet to the remote server as configured in the module's Kconfig.
 * The packet is sent as-is; it is the user's responsibility to ensure the
 * payload is properly formatted.
 *
 * @param[in] packet      Pointer to the packet buffer.
 * @param[in] packet_len  Length of the packet.
 *
 * @return 0 on success, negative value on error.
 */
int cellular_send_packet(const uint8_t *packet, size_t packet_len);

/**
 * @brief Get the current state of the cellular module.
 *
 * Returns the internal state of the cellular subsystem.
 *
 * @return Current cellular state as a value from @ref cellular_state_t.
 */
cellular_state_t cellular_state_get(void);

#endif /* CONFIG_CELLULAR */

#endif /* LIB_CELLULAR_H_ */
