/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrf_modem_at.h>
#include <nrf_modem_gnss.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <date_time.h>

LOG_MODULE_REGISTER(gnss, CONFIG_APP_GNSS_LOG_LEVEL);

static const char update_indicator[] = {'\\', '|', '/', '-'};

static struct nrf_modem_gnss_pvt_data_frame last_pvt;
static uint64_t fix_timestamp;

K_MSGQ_DEFINE(nmea_queue, sizeof(struct nrf_modem_gnss_nmea_data_frame *), 10, 4);
static K_SEM_DEFINE(pvt_data_sem, 0, 1);
static K_SEM_DEFINE(time_sem, 0, 1);

static struct k_poll_event events[2] = {
    K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
                    K_POLL_MODE_NOTIFY_ONLY,
                    &pvt_data_sem, 0),
    K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
                    K_POLL_MODE_NOTIFY_ONLY,
                    &nmea_queue, 0),
};

BUILD_ASSERT(IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS) ||
         IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS) ||
         IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS),
         "CONFIG_LTE_NETWORK_MODE_LTE_M_GPS, "
         "CONFIG_LTE_NETWORK_MODE_NBIOT_GPS or "
         "CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS must be enabled");

static void gnss_event_handler(int event)
{
    int retval;
    struct nrf_modem_gnss_nmea_data_frame *nmea_data;

    switch (event) {
    case NRF_MODEM_GNSS_EVT_PVT:
        retval = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt), NRF_MODEM_GNSS_DATA_PVT);
        if (retval == 0) {
            k_sem_give(&pvt_data_sem);
        }
        break;

    case NRF_MODEM_GNSS_EVT_NMEA:
        nmea_data = k_malloc(sizeof(struct nrf_modem_gnss_nmea_data_frame));
        if (nmea_data == NULL) {
            LOG_ERR("Failed to allocate memory for NMEA");
            break;
        }

        retval = nrf_modem_gnss_read(nmea_data,
                         sizeof(struct nrf_modem_gnss_nmea_data_frame),
                         NRF_MODEM_GNSS_DATA_NMEA);
        if (retval == 0) {
            retval = k_msgq_put(&nmea_queue, &nmea_data, K_NO_WAIT);
        }

        if (retval != 0) {
            k_free(nmea_data);
        }
        break;

    case NRF_MODEM_GNSS_EVT_AGNSS_REQ:
        break;

    default:
        break;
    }
}

static void date_time_evt_handler(const struct date_time_evt *evt)
{
    k_sem_give(&time_sem);
}

static int modem_init(void)
{
    if (IS_ENABLED(CONFIG_DATE_TIME)) {
        date_time_register_handler(date_time_evt_handler);
    }

    return 0;
}

static int gnss_init_and_start(void)
{
    /* Enable GNSS. */
    if (lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS) != 0) {
        LOG_ERR("Failed to activate GNSS functional mode");
        return -1;
    }

    /* Configure GNSS. */
    if (nrf_modem_gnss_event_handler_set(gnss_event_handler) != 0) {
        LOG_ERR("Failed to set GNSS event handler");
        return -1;
    }

    /* Enable all supported NMEA messages. */
    uint16_t nmea_mask = NRF_MODEM_GNSS_NMEA_RMC_MASK |
                 NRF_MODEM_GNSS_NMEA_GGA_MASK |
                 NRF_MODEM_GNSS_NMEA_GLL_MASK |
                 NRF_MODEM_GNSS_NMEA_GSA_MASK |
                 NRF_MODEM_GNSS_NMEA_GSV_MASK;

    if (nrf_modem_gnss_nmea_mask_set(nmea_mask) != 0) {
        LOG_ERR("Failed to set GNSS NMEA mask");
        return -1;
    }

    /* Make QZSS satellites visible in the NMEA output. */
    if (nrf_modem_gnss_qzss_nmea_mode_set(NRF_MODEM_GNSS_QZSS_NMEA_MODE_CUSTOM) != 0) {
        LOG_WRN("Failed to enable custom QZSS NMEA mode");
    }

    /* This use case flag should always be set. */
    uint8_t use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;

    if (IS_ENABLED(CONFIG_GNSS_SAMPLE_MODE_PERIODIC) &&
        !IS_ENABLED(CONFIG_GNSS_SAMPLE_ASSISTANCE_NONE)) {
        /* Disable GNSS scheduled downloads when assistance is used. */
        use_case |= NRF_MODEM_GNSS_USE_CASE_SCHED_DOWNLOAD_DISABLE;
    }

    if (IS_ENABLED(CONFIG_GNSS_SAMPLE_LOW_ACCURACY)) {
        use_case |= NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY;
    }

    if (nrf_modem_gnss_use_case_set(use_case) != 0) {
        LOG_WRN("Failed to set GNSS use case");
    }

    uint16_t fix_retry = CONFIG_GNSS_SAMPLE_PERIODIC_TIMEOUT;
    uint16_t fix_interval = CONFIG_GNSS_SAMPLE_PERIODIC_INTERVAL;

    if (nrf_modem_gnss_fix_retry_set(fix_retry) != 0) {
        LOG_ERR("Failed to set GNSS fix retry");
        return -1;
    }

    if (nrf_modem_gnss_fix_interval_set(fix_interval) != 0) {
        LOG_ERR("Failed to set GNSS fix interval");
        return -1;
    }

    if (nrf_modem_gnss_start() != 0) {
        LOG_ERR("Failed to start GNSS");
        return -1;
    }

    return 0;
}

static void print_satellite_stats(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
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

    printf("Tracking: %2d Using: %2d Unhealthy: %d\n", tracked, in_fix, unhealthy);
}

static void print_flags(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
    if (pvt_data->flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED) {
        printf("GNSS operation blocked by LTE\n");
    }
    if (pvt_data->flags & NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME) {
        printf("Insufficient GNSS time windows\n");
    }
    if (pvt_data->flags & NRF_MODEM_GNSS_PVT_FLAG_SLEEP_BETWEEN_PVT) {
        printf("Sleep period(s) between PVT notifications\n");
    }
    if (pvt_data->flags & NRF_MODEM_GNSS_PVT_FLAG_SCHED_DOWNLOAD) {
        printf("Scheduled navigation data download\n");
    }
}

static void print_fix_data(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
    printf("Latitude           %.06f\n", pvt_data->latitude);
    printf("Longitude:         %.06f\n", pvt_data->longitude);
    printf("Accuracy:          %.01f m\n", (double)pvt_data->accuracy);
    printf("Altitude:          %.01f m\n", (double)pvt_data->altitude);
    printf("Altitude accuracy: %.01f m\n", (double)pvt_data->altitude_accuracy);
    printf("Speed:             %.01f m/s\n", (double)pvt_data->speed);
    printf("Speed accuracy:    %.01f m/s\n", (double)pvt_data->speed_accuracy);
    printf("V. speed:          %.01f m/s\n", (double)pvt_data->vertical_speed);
    printf("V. speed accuracy: %.01f m/s\n", (double)pvt_data->vertical_speed_accuracy);
    printf("Heading:           %.01f deg\n", (double)pvt_data->heading);
    printf("Heading accuracy:  %.01f deg\n", (double)pvt_data->heading_accuracy);
    printf("Date:              %04u-%02u-%02u\n",
           pvt_data->datetime.year,
           pvt_data->datetime.month,
           pvt_data->datetime.day);
    printf("Time (UTC):        %02u:%02u:%02u.%03u\n",
           pvt_data->datetime.hour,
           pvt_data->datetime.minute,
           pvt_data->datetime.seconds,
           pvt_data->datetime.ms);
    printf("PDOP:              %.01f\n", (double)pvt_data->pdop);
    printf("HDOP:              %.01f\n", (double)pvt_data->hdop);
    printf("VDOP:              %.01f\n", (double)pvt_data->vdop);
    printf("TDOP:              %.01f\n", (double)pvt_data->tdop);
}

int gnss_main(void)
{
    int err;
    uint8_t cnt = 0;
    struct nrf_modem_gnss_nmea_data_frame *nmea_data;

    LOG_INF("Starting GNSS sample");

    err = nrf_modem_lib_init();
    if (err) {
        LOG_ERR("Modem library initialization failed, error: %d", err);
        return err;
    }

    if (modem_init() != 0) {
        LOG_ERR("Failed to initialize modem");
        return -1;
    }

    if (gnss_init_and_start() != 0) {
        LOG_ERR("Failed to initialize and start GNSS");
        return -1;
    }

    fix_timestamp = k_uptime_get();

    for (;;) {
        (void)k_poll(events, 2, K_FOREVER);

        if (events[0].state == K_POLL_STATE_SEM_AVAILABLE &&
            k_sem_take(events[0].sem, K_NO_WAIT) == 0) {
            /* New PVT data available */
            printf("\033[1;1H");
            printf("\033[2J");
            print_satellite_stats(&last_pvt);
            print_flags(&last_pvt);
            printf("-----------------------------------\n");

            if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
                fix_timestamp = k_uptime_get();
                print_fix_data(&last_pvt);
            } else {
                printf("Seconds since last fix: %d\n",
                        (uint32_t)((k_uptime_get() - fix_timestamp) / 1000));
                cnt++;
                printf("Searching [%c]\n", update_indicator[cnt%4]);
            }

            printf("\nNMEA strings:\n\n");
        }

        if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE &&
            k_msgq_get(events[1].msgq, &nmea_data, K_NO_WAIT) == 0) {
            /* New NMEA data available */
            printf("%s", nmea_data->nmea_str);
            k_free(nmea_data);
        }

        events[0].state = K_POLL_STATE_NOT_READY;
        events[1].state = K_POLL_STATE_NOT_READY;
    }

    return 0;
}

/* GNSS Thread Parameters */
#define GNSS_STACKSIZE 1024
#define GNSS_PRIORITY 7

K_THREAD_DEFINE(gnss, GNSS_STACKSIZE,
    gnss_main, NULL, NULL, NULL,
    GNSS_PRIORITY, 0, 0);
