#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/net/coap.h>

#include <lib/cellular.h>
#include <lib/coap.h>
#include <zcbor_common.h>

#include <common/command_types.h>
#include <command/czd_command_decode.h>
#include <command/czd_command_types.h>

#include <adc/adcs.h>
#include <imu/imu.h>
#include <temp_hum/temp_hum.h>

LOG_MODULE_REGISTER(remote_commands);

static const struct gpio_dt_spec ls_5vh =
    GPIO_DT_SPEC_GET(DT_NODELABEL(ls_5vh), gpios);

static struct k_work_delayable remote_command_poll_work;
static atomic_t poll_interval_ms = 10000;

static const char * const data_path[] = {
    "ccd99122-3904-454a-95c7-9fb71f2c3fde", "commands", NULL
};

void remote_command_poll_work_handler(struct k_work *work)
{
    if (cellular_state_get() != CELLULAR_STATE_RUNNING) {
        LOG_WRN("Cellular interface not ready. Deferring command poll.");
        goto reschedule;
    }

    uint8_t coap_buf[512] = {0};
    int req_size = coap_build_request(coap_buf, sizeof(coap_buf),
                                      COAP_TYPE_NON_CON,
                                      COAP_METHOD_GET,
                                      data_path,
                                      NULL, 0);

    int err = cellular_send_packet(coap_buf, req_size);
    if (err == 0) {
        LOG_INF("Sent CoAP command GET packet (%d B)", req_size);
    } else {
        LOG_ERR("Failed to send command GET packet: %d", err);
    }

reschedule:
    k_work_schedule(&remote_command_poll_work,
                    K_MSEC(atomic_get(&poll_interval_ms)));
}

int remote_commands_init(void)
{
    k_work_init_delayable(&remote_command_poll_work,
                          remote_command_poll_work_handler);

    k_work_schedule(&remote_command_poll_work,
                    K_NO_WAIT);

    return 0;
}

void remote_commands_set_poll_interval(int32_t interval_ms)
{
    atomic_set(&poll_interval_ms, interval_ms);
    k_work_reschedule(&remote_command_poll_work, K_MSEC(interval_ms));
}

static void update_sensor_poll_rate(enum command_sensors sensor, int32_t val)
{
    switch (sensor) {
    case CMD_SENSOR_ADCS:
        adcs_set_poll_interval(val);
        break;
    case CMD_SENSOR_LIS3MDL:
        lis3mdl_set_poll_interval(val);
        break;
    case CMD_SENSOR_LSM6DSO:
        lsm6dso_set_poll_interval(val);
        break;
    case CMD_SENSOR_SHT4X:
        sht4x_set_poll_interval(val);
        break;
    case CMD_SENSOR_UNKNOWN:
    default:
        LOG_WRN("Unknown sensor type: %d", sensor);
        return;
    }

    LOG_INF("Set poll rate for %s to %d ms",
            command_sensor_to_str(sensor), val);
}

static void update_voltage_rail_state(enum command_rails rail, bool state)
{
    switch (rail) {
    case CMD_RAIL_5VH:
        gpio_pin_set_dt(&ls_5vh, state);
        break;
    case CMD_RAIL_UNKNOWN:
    default:
        LOG_WRN("Unknown rail type: %d", rail);
        return;
    }

    LOG_INF("Set rail %s to state: %s",
            command_rail_to_str(rail), state ? "ON" : "OFF");
}

static void issue_delete_request(void)
{
    uint8_t coap_buf[256] = {0};
    int req_size = coap_build_request(coap_buf, sizeof(coap_buf),
                                      COAP_TYPE_NON_CON,
                                      COAP_METHOD_DELETE,
                                      data_path,
                                      NULL, 0);

    int err = cellular_send_packet(coap_buf, req_size);
    if (err) {
        LOG_ERR("Failed to send command DELETE packet: %d", err);
        return;
    }
    LOG_INF("Sent CoAP command DELETE packet (%d B)", req_size);
}

static void process_remote_command(struct command *cmd)
{
    if (!cmd_is_valid_type(cmd->ty)) {
        LOG_WRN("Invalid command type: %u", cmd->ty);
        return;
    }

    switch (cmd->ty) {
    case CMD_NONE_AVAILABLE:
        LOG_INF("No commands available!");
        return;

    case CMD_SET_POLL_RATE:
        if (!cmd->i_present) {
            LOG_WRN("SET_POLL_RATE missing 'i' field");
            return;
        }
        if (!cmd_is_valid_sensor(cmd->ta)) {
            LOG_WRN("Invalid sensor target: %u", cmd->ta);
            return;
        }

        update_sensor_poll_rate(cmd->ta, cmd->i.i);
        break;

    case CMD_SET_RAIL_STATE:
        if (!cmd->b_present) {
            LOG_WRN("SET_RAIL_STATE missing 'b' field");
            return;
        }
        if (!cmd_is_valid_rail(cmd->ta)) {
            LOG_WRN("Invalid rail target: %u", cmd->ta);
            return;
        }

        update_voltage_rail_state(cmd->ta, cmd->b.b);
        break;

    default:
        LOG_WRN("Unhandled command type: %u", cmd->ty);
        return;
    }

    issue_delete_request();
}


void remote_commands_handle_packet(const uint8_t *buf, size_t len)
{
    struct coap_packet response;
    int err = 0;

    err = coap_packet_parse(&response, (uint8_t *)buf, len, NULL, 0);
    if (err) {
        LOG_ERR("Failed to parse CoAP packet: %d", err);
        return;
    }

    uint16_t payload_len = 0;
    const uint8_t *payload_ptr = coap_packet_get_payload(&response, &payload_len);
    if (payload_ptr == NULL) {
        LOG_ERR("Failed to extract CoAP payload: %d", err);
        return;
    }

    struct command cmd;
    err = cbor_decode_command(payload_ptr, payload_len, &cmd, NULL);
    if (err != ZCBOR_SUCCESS) {
        LOG_WRN("Response was not a remote command");
        return;
    }

    process_remote_command(&cmd);
}
