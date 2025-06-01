
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zcbor_common.h>

#include <zephyr/net/coap.h>

#include <lib/cellular.h>
#include <lib/coap.h>
#include <datapoint/czd_datapoint_encode.h>
#include <datapoint/czd_datapoint_types.h>

#include <date_time.h>

#include <datapoint_queue.h>

LOG_MODULE_REGISTER(datapoint, LOG_LEVEL_ERR);

const char * const data_path[] = {
    "5a988ea0-ed80-4a54-9ad8-870cef2e9942", "data", NULL
};

static void send_dp(struct datapoint *dp)
{
    if (cellular_state_get() != CELLULAR_STATE_RUNNING) {
        LOG_WRN("Cellular interface not ready. Dropping datapoint");
        return;
    }

    uint8_t payload[100];
    size_t payload_len = 0;

    /* Set IMEI tail */
    dp->i = 267864;

    /* Set timestamp */
    if (date_time_now(&dp->t) != 0) {
        LOG_ERR("Failed to get timestamp.");
        return;
    }
    dp->t /= 1000; /* Convert to seconds */

    int err = cbor_encode_datapoint(payload, sizeof(payload), dp, &payload_len);
    if (err != ZCBOR_SUCCESS) {
        LOG_ERR("CBOR encode fail: %d", err);
        return;
    }

    uint8_t coap_buf[1024] = {0};
    int req_size = coap_build_request(coap_buf, sizeof(coap_buf),
                                      COAP_TYPE_NON_CON,
                                      COAP_METHOD_POST,
                                      data_path,
                                      payload, payload_len);
    LOG_INF("Sending CoAP packet (%d B): %.*s", req_size,
            (int)dp->s.len, dp->s.value);

    cellular_send_packet(coap_buf, req_size);
}

int datapoint_thread(void)
{
    struct datapoint dp;

    while (1) {
        if (datapoint_dequeue(&dp, K_MSEC(1000)) == 0) {
            send_dp(&dp);
        }
    }

	return 0;
}

#define DATAPOINT_STACKSIZE 2048
#define DATAPOINT_PRIORITY  6

K_THREAD_DEFINE(datapoint, DATAPOINT_STACKSIZE, datapoint_thread, NULL, NULL,
                NULL, DATAPOINT_PRIORITY, 0, 0);
