#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zcbor_common.h>

#include <zephyr/net/coap.h>

#include <lib/cellular.h>
#include <lib/coap.h>
#include <datapoint/czd_datapoint_encode.h>
#include <datapoint/czd_datapoint_types.h>

#include <date_time.h>

#include <datapoint_helpers.h>
#include <datapoint_queue.h>

#include <sd_card.h>

LOG_MODULE_REGISTER(datapoint, LOG_LEVEL_INF);

const char * const data_path[] = {
    "ccd99122-3904-454a-95c7-9fb71f2c3fde", "data", NULL
};

static void dp_sink_cellular(struct datapoint *dp)
{
    if (cellular_state_get() != CELLULAR_STATE_RUNNING) {
        LOG_WRN("Cellular interface not ready. Datapoint not sent");
        return;
    }

    uint8_t payload[100];
    size_t payload_len = 0;

    int err = cbor_encode_datapoint(payload, sizeof(payload), dp, &payload_len);
    if (err != ZCBOR_SUCCESS) {
        LOG_ERR("CBOR encode fail: %d", err);
        return;
    }

    uint8_t coap_buf[512] = {0};
    int req_size = coap_build_request(coap_buf, sizeof(coap_buf),
                                      COAP_TYPE_NON_CON,
                                      COAP_METHOD_POST,
                                      data_path,
                                      payload, payload_len);
    LOG_INF("Sending CoAP packet (%d B): %.*s", req_size,
            (int)dp->s.len, dp->s.value);

    cellular_send_packet(coap_buf, req_size);
}

static void dp_sink_sd_card(struct datapoint *dp) {
    char line_buf[256] = {0};
    size_t line_len = datapoint_to_csv(dp, line_buf, sizeof(line_buf));
    sd_card_submit_line(line_buf, line_len, K_NO_WAIT);
}

int datapoint_thread(void)
{
    struct datapoint dp;

    while (1) {
        if (datapoint_dequeue(&dp, K_MSEC(1000)) == 0) {
            /* Set IMEI tail */
            dp.i = 267864;

            /* Set timestamp */
            if (date_time_now(&dp.t) != 0) {
                LOG_ERR("Failed to get timestamp.");
            }
            dp.t /= 1000; /* Convert to seconds */

            dp_sink_cellular(&dp);
            dp_sink_sd_card(&dp);
        }
    }

	return 0;
}

#define DATAPOINT_STACKSIZE 2048
#define DATAPOINT_PRIORITY  6

K_THREAD_DEFINE(datapoint, DATAPOINT_STACKSIZE, datapoint_thread, NULL, NULL,
                NULL, DATAPOINT_PRIORITY, 0, 0);
