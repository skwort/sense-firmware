#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrf_modem_gnss.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>

#include "state.h"

#ifdef CONFIG_APP_USE_GNSS

LOG_MODULE_REGISTER(gnss, CONFIG_APP_GNSS_LOG_LEVEL);

static struct nrf_modem_gnss_pvt_data_frame last_pvt;

/* Internal state tracking */
#define GNSS_STATE_SEARCH   0
#define GNSS_STATE_SLEEP   1

static volatile uint8_t gnss_internal_state = GNSS_STATE_SEARCH;

/* Use a semaphore to signal GNSS data availability */
static K_SEM_DEFINE(pvt_data_sem, 0, 1);

static void gnss_event_handler(int event)
{
    int retval;

    switch (event) {
    /* The EVT_PVT event is sent every second while the modem is running. */
    case NRF_MODEM_GNSS_EVT_PVT:
        retval = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt),
                                     NRF_MODEM_GNSS_DATA_PVT);
        if (retval == 0)
            k_sem_give(&pvt_data_sem);
        break;

    /* The EVT_FIX event is sent only when a fix is acquired. */
    case NRF_MODEM_GNSS_EVT_FIX:
        retval = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt),
                                     NRF_MODEM_GNSS_DATA_PVT);
        if (retval == 0)
            k_sem_give(&pvt_data_sem);
        break;

    case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
        LOG_INF("GNSS modem waking from periodic sleep.");
        gnss_internal_state = GNSS_STATE_SEARCH;
        break;

    case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
        LOG_INF("GNSS modem sleeping after periodic mode fix timeout.");
        gnss_internal_state = GNSS_STATE_SLEEP;
        break;

    case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
        LOG_INF("GNSS modem sleeping after periodic mode fix acquired.");
        gnss_internal_state = GNSS_STATE_SLEEP;
        break;

    default:
        break;
    }
}

static int gnss_init_and_start(struct state *state)
{
    if (lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS) != 0) {
        LOG_ERR("Failed to activate GNSS functional mode");
        return -1;
    }

    if (nrf_modem_gnss_event_handler_set(gnss_event_handler) != 0) {
        LOG_ERR("Failed to set GNSS event handler");
        return -1;
    }

    /* This use case flag is required by default. */
    uint8_t use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;

    if (nrf_modem_gnss_use_case_set(use_case) != 0) {
        LOG_WRN("Failed to set GNSS use case");
    }

    state->gnss_fix_retry_period = CONFIG_APP_GNSS_DEFAULT_PERIODIC_TIMEOUT;
    state->gnss_fix_interval = CONFIG_APP_GNSS_DEFAULT_FIX_INTERVAL;

    /* Give the GNSS receiver fix_retry seconds to find a fix.
     * NOTE: Until the modem finds its first fix, the fix_retry period will
     * remain 60 seconds. If the value is set to 0, then it will search
     * indefinitely*/
    if (nrf_modem_gnss_fix_retry_set(state->gnss_fix_retry_period) != 0) {
        LOG_ERR("Failed to set GNSS fix retry");
        return -1;
    }

    /* Look for a fix every fix_interval seconds.
     * NOTE: it appears that if the the first fix hasn't been found, then
     * the fix interval time is given by:
     *     next_wake_time = current_time + (fix_interval - fix_retry)
     * Otherwise, the modem appears to respect the fix interval, with some
     * understandable RTC skew */
    if (nrf_modem_gnss_fix_interval_set(state->gnss_fix_interval) != 0) {
        LOG_ERR("Failed to set GNSS fix interval");
        return -1;
    }

    state->gnss_last_update_time = 0;

    if (nrf_modem_gnss_start() != 0) {
        LOG_ERR("Failed to start GNSS");
        return -1;
    }

    return 0;
}

static void log_satellite_stats(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
    uint8_t tracked   = 0;
    uint8_t in_fix    = 0;
    uint8_t unhealthy = 0;

    for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; ++i) {
        if (pvt_data->sv[i].sv > 0) {
            tracked++;

            if (pvt_data->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX) {
                in_fix++;
            }

            if (pvt_data->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY) {
                unhealthy++;
            }
        }
    }
    LOG_DBG("Tracking: %2d Using: %2d Unhealthy: %d\n",
            tracked, in_fix, unhealthy);
}

static void process_fix_data(struct nrf_modem_gnss_pvt_data_frame *pvt_data,
                             struct state *s)
{
    /* Push PVT data to system state struct */
    s->gnss_latitude = pvt_data->latitude;
    s->gnss_longitude = pvt_data->longitude;
    s->gnss_accuracy = pvt_data->accuracy;
    s->gnss_altitude = pvt_data->altitude;
    s->gnss_altitude_accuracy = pvt_data->altitude_accuracy;
    s->gnss_speed = pvt_data->speed;
    s->gnss_speed_accuracy = pvt_data->speed_accuracy;
    s->gnss_vertical_speed = pvt_data->vertical_speed;
    s->gnss_vertical_speed_accuracy = pvt_data->vertical_speed_accuracy;
    s->gnss_heading = pvt_data->heading;
    s->gnss_heading_accuracy = pvt_data->heading_accuracy;

    LOG_INF("Latitude           %.06f", pvt_data->latitude);
    LOG_INF("Longitude:         %.06f", pvt_data->longitude);
    LOG_INF("Accuracy:          %.01f m", (double)pvt_data->accuracy);
    LOG_DBG("Altitude:          %.01f m", (double)pvt_data->altitude);
    LOG_DBG("Altitude accuracy: %.01f m", (double)pvt_data->altitude_accuracy);
    LOG_DBG("Speed:             %.01f m/s", (double)pvt_data->speed);
    LOG_DBG("Speed accuracy:    %.01f m/s", (double)pvt_data->speed_accuracy);
    LOG_DBG("V. speed:          %.01f m/s", (double)pvt_data->vertical_speed);
    LOG_DBG("V. speed accuracy: %.01f m/s", (double)pvt_data->vertical_speed_accuracy);
    LOG_DBG("Heading:           %.01f deg", (double)pvt_data->heading);
    LOG_DBG("Heading accuracy:  %.01f deg", (double)pvt_data->heading_accuracy);
    LOG_DBG("Date:              %04u-%02u-%02u",
           pvt_data->datetime.year,
           pvt_data->datetime.month,
           pvt_data->datetime.day);
    LOG_DBG("Time (UTC):        %02u:%02u:%02u.%03u",
           pvt_data->datetime.hour,
           pvt_data->datetime.minute,
           pvt_data->datetime.seconds,
           pvt_data->datetime.ms);
}

int gnss_init(struct state *state)
{
    int err;

    LOG_INF("Initialsing GNSS");

    err = nrf_modem_lib_init();
    if (err) {
        LOG_ERR("Modem library initialisation failed, error: %d", err);
        return err;
    }

    if (gnss_init_and_start(state) != 0) {
        LOG_ERR("Failed to initialise and start GNSS");
        return -1;
    }

    return 0;
}

void poll_gnss(struct state *state)
{
    if (gnss_internal_state == GNSS_STATE_SLEEP)
        return;

    while (k_sem_take(&pvt_data_sem, K_NO_WAIT) == 0) {
        log_satellite_stats(&last_pvt);

        if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
            LOG_INF("Updating GNSS data.");
            process_fix_data(&last_pvt, state);
            state->gnss_last_update_time = k_uptime_get();
        } else if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_SCHED_DOWNLOAD) {
            LOG_INF("GNSS scheduled download in progress.");
        } else if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED) {
            LOG_WRN("GNSS operation blocked by LTE\n");
        } else if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME) {
            LOG_WRN("Insufficient GNSS time windows\n");
        } else {
            LOG_INF("GNSS attempting to acquire fix...");
        }
    }
}

#endif /* CONFIG_APP_USE_GNSS */
