#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>

#include <lib/cellular.h>

#include "cellular_queue.h"
#include "cellular_backend.h"
#include "cellular_packet.h"

LOG_MODULE_REGISTER(cellular, CONFIG_CELLULAR_LOG_LEVEL);

static int remote_server_socket;
static struct sockaddr_storage remote_server;

struct k_thread cellular_thread;
static k_tid_t cellular_thread_id;
static void cellular_thread_function(void *p1, void *p2, void *p3);

K_THREAD_STACK_DEFINE(cellular_thread_stack,
                      CONFIG_CELLULAR_THREAD_STACK_SIZE);

static const struct cellular_backend *backend;

cellular_recv_cb_t recv_callback;

static atomic_t cellular_state = ATOMIC_INIT(CELLULAR_STATE_UNINITIALISED);

cellular_state_t cellular_state_get(void)
{
    return (cellular_state_t)atomic_get(&cellular_state);
}

static inline void cellular_state_set(cellular_state_t state)
{
    (void)atomic_set(&cellular_state, (atomic_val_t)state);
}

/**
 * @brief Initialise the cellular module.
 *
 * This function selects the appropriate cellular backend implementation based
 * on  build-time configuration (e.g. CONFIG_CELLULAR_BACKEND_NRF) and starts
 * the cellular thread.
 *
 * Only one backend is active per build. Backend selection is internal and
 * not exposed via the public API.
 *
 * @return 0 on success, or negative error code.
 */
int cellular_init(cellular_recv_cb_t cb)
{
    cellular_state_t state = cellular_state_get();

    if (state == CELLULAR_STATE_STARTING || state == CELLULAR_STATE_RUNNING) {
        LOG_ERR("Cellular thread is already running.");
        return -EBUSY;
    }

    if (cb == NULL) {
        LOG_ERR("Receive callback is invalid.");
        cellular_state_set(CELLULAR_STATE_UNINITIALISED);
        return -EINVAL;
    }

    recv_callback = cb;

    backend = cellular_get_selected_backend();
    if (backend == NULL) {
        LOG_ERR("Cellular backend is invalid.");
        cellular_state_set(CELLULAR_STATE_UNINITIALISED);
        return -EINVAL;
    }

    cellular_thread_id = k_thread_create(&cellular_thread,
                                         cellular_thread_stack,
                                         CONFIG_CELLULAR_THREAD_STACK_SIZE,
                                         cellular_thread_function,
                                         NULL, NULL, NULL,
                                         CONFIG_CELLULAR_THREAD_PRIORITY,
                                         0,
                                         K_NO_WAIT);

    cellular_state_set(CELLULAR_STATE_STARTING);

    LOG_INF("Cellular thread created.");

    return 0;
}

int cellular_send_packet(const uint8_t *data, size_t data_len)
{
    if (data == NULL ||
            data_len <= 0 ||
            data_len > CONFIG_CELLULAR_UPLINK_BUFFER_SIZE) {
        return -EINVAL;
    }

    int ret = 0;
    struct cellular_packet *packet = NULL;

    ret = cellular_packet_alloc(&packet);
    if (ret != 0) {
        LOG_ERR("Failed to alloc packet: %d", ret);
        return ret;
    }

    memcpy(packet->buffer, data, data_len);
    packet->len = data_len;

    return cellular_uplink_enqueue(&packet, K_NO_WAIT);
}

static int resolve_remote_server(void)
{
    int err;
    struct addrinfo *result;
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM
    };

    err = getaddrinfo(CONFIG_REMOTE_SERVER_HOSTNAME, NULL, &hints, &result);
    if (err != 0) {
		LOG_ERR("getaddrinfo failed: %d", err);
		return -EIO;
	}

	if (result == NULL) {
		LOG_ERR("Address not found.");
		return -ENOENT;
	}

    struct sockaddr_in *server = ((struct sockaddr_in *)&remote_server);

    server->sin_addr.s_addr =
            ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
    server->sin_family = AF_INET;
    server->sin_port = htons(CONFIG_REMOTE_SERVER_PORT);

    char ipv4_addr[NET_IPV4_ADDR_LEN];

    inet_ntop(AF_INET, &server->sin_addr.s_addr, ipv4_addr, sizeof(ipv4_addr));
    LOG_INF("Found IPv4 address of remote server: %s", ipv4_addr);

    freeaddrinfo(result);

    return 0;
}

/** @brief Setup socket */
static int initialise_socket(void)
{
    int err;

    remote_server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (remote_server_socket < 0) {
        LOG_ERR("Failed to create socket for remote server: %d", errno);
        return -errno;
    }

    err = fcntl(remote_server_socket, F_SETFL, O_NONBLOCK);
    if (err != 0) {
        close(remote_server_socket);
        LOG_ERR("Failed to set socket as non-blocking: %d", err);
        return err;
    }

    err = connect(remote_server_socket,
                  (struct sockaddr *)&remote_server,
                  sizeof(struct sockaddr_in));
    if (err != 0) {
        close(remote_server_socket);
        LOG_ERR("Failed to connect: %d", errno);
        return -errno;
    }

    return 0;
}

static int init_cellular_stack(void)
{
    int err;

    err = backend->init();
    if (err != 0) {
        LOG_ERR("Failed to initialise backend.");
        cellular_state_set(CELLULAR_STATE_BACKEND_ERROR);
        return err;
    }

    err = resolve_remote_server();
    if (err != 0) {
        LOG_ERR("Failed to resolve remote server.");
        cellular_state_set(CELLULAR_STATE_REMOTE_SERVER_ERROR);
        return err;
    }

    err = initialise_socket();
    if (err != 0) {
        LOG_ERR("Failed to initialise socket.");
        cellular_state_set(CELLULAR_STATE_SOCKET_ERROR);
        return err;
    }

    return 0;
}

static void cellular_thread_function(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    if (init_cellular_stack() != 0 ) {
        LOG_ERR("Cellular init failed. Thread exiting.");
        return;
    }

    cellular_state_set(CELLULAR_STATE_RUNNING);

    ssize_t num_bytes = 0;
    struct cellular_packet *uplink_packet;
    uint8_t rx_buffer[CONFIG_CELLULAR_DOWNLINK_BUFFER_SIZE] = {0};

    while (true) {

        if (cellular_uplink_dequeue(&uplink_packet, K_NO_WAIT) == 0) {

            num_bytes = send(remote_server_socket,
                             uplink_packet->buffer,
                             uplink_packet->len,
                             0);

            if (num_bytes == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    LOG_INF("Operation would block. Message not sent.");
                } else {
                    LOG_ERR("Error in send: %d", errno);
                }
            } else {
                LOG_INF ("Sent uplink bytes: %d", num_bytes);
            }

            cellular_packet_free(uplink_packet);
        }

        num_bytes = recv(remote_server_socket,
                         rx_buffer, sizeof(rx_buffer), 0);

        if (num_bytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                LOG_DBG("No data available on socket.");
            } else {
                LOG_ERR("Error in recv: %d", errno);
            }

        } else {
            LOG_INF("Downlink bytes received: %d", num_bytes);
            recv_callback(rx_buffer, num_bytes);
        }

        k_sleep(K_MSEC(5));
    }
}

