#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include <datapoint_queue.h>
#include <datapoint_helpers.h>
#include <datapoint/czd_datapoint_types.h>

int get_and_submit_sensor_datapoint(const struct device *dev,
                                    enum sensor_channel chan,
                                    const char *label,
                                    const char *unit)
{
    struct sensor_value val;
    int err = sensor_channel_get(dev, chan, &val);
    if (err != 0) {
        return 1;
    }

    struct datapoint dp = {
        .f_present = true,
        .r_present = false,
        .u_present = true,
        .s = {.value = label, .len = strlen(label)},
        .u.u = {.value = unit, .len = strlen(unit)},
        .f.f = sensor_value_to_double(&val),
    };

    return datapoint_enqueue(&dp, K_NO_WAIT);
}

int submit_float_datapoint(double value,
                           const char *label,
                           const char *unit)
{
    struct datapoint dp = {
        .f_present = true,
        .r_present = false,
        .u_present = true,
        .s = {.value = label, .len = strlen(label)},
        .u.u = {.value = unit, .len = strlen(unit)},
        .f.f = value,
    };

    return datapoint_enqueue(&dp,K_NO_WAIT);
}
