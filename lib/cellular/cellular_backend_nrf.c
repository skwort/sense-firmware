#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <nrf_errno.h>
#include <nrf_socket.h>

#include "cellular_backend.h"

LOG_MODULE_DECLARE(cellular);

K_SEM_DEFINE(lte_connected_sem, 0, 1);

static void lte_event_handler(const struct lte_lc_evt *const evt)
{
    switch (evt->type) {
    case LTE_LC_EVT_NW_REG_STATUS:
        if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
            (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
            break;
        }
		LOG_INF("Network registration status: %s",
				evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
				"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected_sem);
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s", evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
				"Connected" : "Idle");
		break;
     default:
             break;
     }
}

static int initialise_modem(void)
{
    int err;

    err = nrf_modem_lib_init();
    if (err == -1) {
        LOG_WRN("Modem already initialised.");
    }  else if (err != 0) {
        LOG_ERR("Failed to initialise modem library: %d", err);
        return err;
    }

    lte_lc_register_handler(lte_event_handler);

    err =  lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_LTE);
    if (err != 0) {
        LOG_ERR("Failed to activate LTE in modem: %d", err);
        return err;
    }

    LOG_ERR("Waiting for LTE connected semaphore.");
	err = k_sem_take(&lte_connected_sem, K_MSEC(MSEC_PER_SEC * CONFIG_CELLULAR_CONN_TIMEOUT));
    if (err != 0) {
        LOG_ERR("Failed to take LTE connection semaphore: %d", err);
        return err;
    }

    struct nrf_in_addr dns;
    dns.s_addr = 16843009;  // Cloudflare DNS, 1.1.1.1
    nrf_setdnsaddr(NRF_AF_INET, &dns, sizeof(dns));
    LOG_INF("Manually set DNS server via nrf_setdnsaddr");

    LOG_ERR("LTE connection established.");

    return 0;
}

const struct cellular_backend cellular_backend_nrf = {
    .init = initialise_modem,
};

const struct cellular_backend *cellular_get_selected_backend(void)
{
    return &cellular_backend_nrf;
}
