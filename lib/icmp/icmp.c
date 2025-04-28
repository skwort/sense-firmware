#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lib/icmp.h>

#include "icmp_queue.h"
#include "icmp_frame.h"
#include "icmp_phy.h"

LOG_MODULE_REGISTER(icmp, CONFIG_ICMP_LOG_LEVEL);

icmp_callback_t rx_dispatch_cb[CONFIG_ICMP_MAX_TARGETS] = {0};

/* The ICMP inflight mutex is used to protect the state of the inflight
 * bitmap and inflight table. */
K_MUTEX_DEFINE(icmp_inflight_mutex);

#if CONFIG_ICMP_MAX_INFLIGHT_MSGS_8
#define ICMP_MAX_INFLIGHT_MSGS 8
static uint8_t inflight_bitmap;
#elif CONFIG_ICMP_MAX_INFLIGHT_MSGS_16
#define ICMP_MAX_INFLIGHT_MSGS 16
static uint16_t inflight_bitmap;
#elif CONFIG_ICMP_MAX_INFLIGHT_MSGS_32
#define ICMP_MAX_INFLIGHT_MSGS 32
static uint32_t inflight_bitmap;
#else
#error "Maximum number of ICMP inflight messages is undefined."
#endif

#define IS_BIT_SET(bitmap, bit_num)  ((bitmap) & (1U << (bit_num)))

struct icmp_inflight_table_entry {
    int64_t timestamp;
    icmp_response_cb_t callback;
    void *user_data;
};

struct icmp_inflight_table_entry
        icmp_inflight_table[ICMP_MAX_INFLIGHT_MSGS] = {0};

K_THREAD_STACK_DEFINE(icmp_thread_stack, CONFIG_ICMP_THREAD_STACK_SIZE);
struct k_thread icmp_thread;
static k_tid_t icmp_thread_id;
void icmp_thread_function(void *p1, void *p2, void *p3);

const struct icmp_phy_api *phy_api;

/**
 * @brief Construct and send an ICMP frame.
 *
 * This private method allocates an ICMP frame, populates the fields, and
 * adds it to the ICMP TX queue.
 */
static int icmp_send_frame(uint8_t type,
                           uint8_t msg_id,
                           uint8_t target_id,
                           const uint8_t *payload,
                           size_t payload_len)
{
    struct icmp_frame *frame = NULL;
    int ret = icmp_frame_alloc(&frame);
    if (ret != 0) {
        return ret;
    }

    frame->type = type;
    frame->msg_id = msg_id;
    frame->target = target_id;
    frame->length = payload_len;
    memcpy(frame->payload, payload, payload_len);

    return icmp_tx_enqueue(&frame, K_NO_WAIT);
}

static int allocate_msg_id(uint32_t bitmap, int bitmap_len)
{
    int bit = 0;

    for (bit = 0; bit < bitmap_len; bit++) {
        if ((bitmap & 1) == 0) {
            return bit;
        }
        bitmap >>= 1;
    }

    return -EAGAIN;
}

#ifdef CONFIG_ICMP_TESTING
void icmp_test_reset_inflight_state(void)
{
    k_mutex_lock(&icmp_inflight_mutex, K_FOREVER);
    inflight_bitmap = 0;
    memset(icmp_inflight_table, 0, sizeof(icmp_inflight_table));
    k_mutex_unlock(&icmp_inflight_mutex);
}
#endif

// ==== Public API ============================================================

/**
 * @brief Initialise the ICMP server
 *
 * This function loads the user-chosen PHY and starts the ICMP server thread.
 */
int icmp_init(void)
{
    /* Load the selected PHY */
    phy_api = icmp_get_selected_phy();
    if (phy_api == NULL) {
        LOG_ERR("Invalid PHY API provided.");
        return -EINVAL;
    }

    icmp_thread_id = k_thread_create(&icmp_thread,
                                     icmp_thread_stack,
                                     CONFIG_ICMP_THREAD_STACK_SIZE,
                                     icmp_thread_function,
                                     NULL, NULL, NULL,
                                     CONFIG_ICMP_THREAD_PRIORITY,
                                     0,
                                     K_NO_WAIT);

    LOG_INF("ICMP thread created.");

    return 0;
}

int icmp_register_target(uint8_t target_id, icmp_callback_t callback)
{
    if (target_id < 0 || target_id >= CONFIG_ICMP_MAX_TARGETS) {
        return -EINVAL;
    }

    if (callback == NULL) {
        return -EINVAL;
    }

    rx_dispatch_cb[target_id] = callback;
    return 0;
}


int icmp_command(uint8_t target_id,
                 const uint8_t *payload,
                 size_t payload_len,
                 icmp_response_cb_t cb,
                 void *user_data)
{
    if (payload_len > ICMP_MAX_PAYLOAD_SIZE) {
        return -EINVAL;
    }

    /* Create the inflight table entry */
    int ret = k_mutex_lock(&icmp_inflight_mutex, K_NO_WAIT);
    if (ret != 0) {
        LOG_ERR("Unable to lock icmp_inflight_mutex: %d", ret);
        return ret;
    }

    /* Generate msg_id */
    int msg_id = allocate_msg_id(inflight_bitmap, ICMP_MAX_INFLIGHT_MSGS);
    if (msg_id < 0) {
        LOG_ERR("Unable to allocate message id: %d", msg_id);
        k_mutex_unlock(&icmp_inflight_mutex);
        return msg_id;
    }

    inflight_bitmap |= (1 << msg_id);
    icmp_inflight_table[msg_id].timestamp = 0;
    icmp_inflight_table[msg_id].callback = cb;
    icmp_inflight_table[msg_id].user_data = user_data;
    k_mutex_unlock(&icmp_inflight_mutex);

    return icmp_send_frame(ICMP_TYPE_COMMAND,
                           msg_id,
                           target_id,
                           payload,
                           payload_len);
}

int icmp_respond(uint8_t target_id,
                 uint8_t msg_id,
                 const uint8_t *payload,
                 size_t payload_len)
{
    if (payload_len > ICMP_MAX_PAYLOAD_SIZE) {
        return -EINVAL;
    }

    return icmp_send_frame(ICMP_TYPE_RESPONSE,
                           msg_id,
                           target_id,
                           payload,
                           payload_len);
}

int icmp_notify(uint8_t target_id,
                const uint8_t *payload,
                size_t payload_len)
{
    if (payload_len > ICMP_MAX_PAYLOAD_SIZE) {
        return -EINVAL;
    }

    /* NOTE: We use 255 as a dummy message id for NOTIFY frames. */
    return icmp_send_frame(ICMP_TYPE_NOTIFY,
                           255,
                           target_id,
                           payload,
                           payload_len);
}


// ====  ICMP Server and Dispatch Logic  ======================================

/* Frame reception is handled via registered callback functions. The process of
 * calling the callback function is referred to as dispatching the frame.
 *
 * Users of the API register a callback for a particular target. This callback
 * is invoked when frames addressed to that target are received.
 *
 * If an incoming RESPONSE message has a response_callback associated with
 * its message ID, the registered response_callback (from the
 * icmp_inflight_table) will instead be called.
 *
 * Callbacks are executed on the system workqueue using the
 * icmp_dispatch_handler function. This function uses the singleton
 * icmp_work_ctx variable to pass the callback and icmp_frame to the dispatch
 * handler. We use a semaphore to signal the availability of the singleton.
 * After executing the callback, the dispatch handler will free the icmp_frame.
 */

static struct icmp_work_ctx {
    struct k_work work;
    struct icmp_frame *frame;
    icmp_callback_t icmp_cb;
} icmp_work_ctx;

K_SEM_DEFINE(icmp_work_sem, 1, 1)

static void icmp_dispatch_handler(struct k_work *item)
{
    struct icmp_work_ctx *ctx =
        CONTAINER_OF(item, struct icmp_work_ctx, work);

    struct icmp_frame *frame = ctx->frame;

     if (frame == NULL) {
        LOG_ERR("Received NULL frame. Dropping message.");
        k_sem_give(&icmp_work_sem);
        return;
    }

    int ret = k_mutex_lock(&icmp_inflight_mutex, K_MSEC(30));
    if (ret != 0) {
        LOG_ERR("ICMP dispatch handler failed to lock inflight mutex. "
                "Dropping message.");
        goto free;
    }

    /* Trigger the response callback for the given msg_id if it exists */
    if (frame->type == ICMP_TYPE_RESPONSE &&
        IS_BIT_SET(inflight_bitmap, frame->msg_id) &&
        icmp_inflight_table[frame->msg_id].callback != NULL) {
        k_mutex_unlock(&icmp_inflight_mutex);

        struct icmp_inflight_table_entry te =
                icmp_inflight_table[frame->msg_id];

        LOG_INF("Triggering response callback for msg_id %d", frame->msg_id);
        te.callback(frame->payload, frame->length, te.user_data);

        /* Unset the inflight bit and remove the table entry */
        k_mutex_lock(&icmp_inflight_mutex, K_NO_WAIT);
        inflight_bitmap &= ~(1 << frame->msg_id);
        icmp_inflight_table[frame->msg_id].timestamp = 0;
        icmp_inflight_table[frame->msg_id].callback = NULL;
        icmp_inflight_table[frame->msg_id].user_data = NULL;
        k_mutex_unlock(&icmp_inflight_mutex);

    } else {
        k_mutex_unlock(&icmp_inflight_mutex);
        /* Trigger the default target callback */
        if (ctx->icmp_cb == NULL) {
            LOG_ERR("Dispatch callback is invalid. Dropping message.");
        } else {
            LOG_INF("Triggering dispatch callback");
            ctx->icmp_cb(frame->payload, frame->length);
        }
    }

free:
    /* Free the frame */
    icmp_frame_free(ctx->frame);
    LOG_INF("ICMP frame free'd");

    /* Signal work availability */
    k_sem_give(&icmp_work_sem);
}

static void icmp_dispatch(struct icmp_frame *frame)
{
    /* Setup the ctx */
    icmp_work_ctx.work.handler = icmp_dispatch_handler;
    icmp_work_ctx.frame = frame;
    icmp_work_ctx.icmp_cb = rx_dispatch_cb[frame->target];

    /* Issue the work item */
    k_work_submit(&icmp_work_ctx.work);
}

static inline void update_inflight_timestamp(struct icmp_frame *frame)
{
    /* Store the send timestamp */
    if (frame->type == ICMP_TYPE_COMMAND) {
        k_mutex_lock(&icmp_inflight_mutex, K_NO_WAIT);
        icmp_inflight_table[frame->msg_id].timestamp = k_uptime_get();
        k_mutex_unlock(&icmp_inflight_mutex);
    }
}

/**
 * @brief The ICMP thread function.
 *
 * This function contains the ICMP server logic. At a high level, this function
 * reads on the TX and RX queues, and dispatches received frames to the
 * appropriate location.
 */
void icmp_thread_function(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    int ret = phy_api->init();
    if (ret != 0) {
        LOG_ERR("ICMP initialisation failed: %d", ret);
        return;
    }

    while (true) {

        struct icmp_frame *tx_frame, *rx_frame;
        ret = icmp_tx_dequeue(&tx_frame, K_NO_WAIT);
        if (ret == 0) {
            update_inflight_timestamp(tx_frame);
            phy_api->send(tx_frame);
            icmp_frame_free(tx_frame);
        }

        if (k_sem_take(&icmp_work_sem, K_NO_WAIT) == 0) {
            ret = icmp_rx_dequeue(&rx_frame, K_NO_WAIT);
            if (ret == 0) {
                icmp_dispatch(rx_frame);
            } else {
                k_sem_give(&icmp_work_sem);
            }
        }

        k_sleep(K_MSEC(5));
    }
}
