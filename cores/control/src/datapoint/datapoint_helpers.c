#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <string.h>

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
        .n_present = false,
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
        .n_present = false,
        .f_present = true,
        .r_present = false,
        .u_present = true,
        .s = {.value = label, .len = strlen(label)},
        .u.u = {.value = unit, .len = strlen(unit)},
        .f.f = value,
    };

    return datapoint_enqueue(&dp,K_NO_WAIT);
}

int submit_int_datapoint(int32_t value,
                         const char *label,
                         const char *unit)
{
    struct datapoint dp = {
        .n_present = true,
        .f_present = false,
        .r_present = false,
        .u_present = true,
        .s = {.value = label, .len = strlen(label)},
        .u.u = {.value = unit, .len = strlen(unit)},
        .n.n = value,
    };

    return datapoint_enqueue(&dp,K_NO_WAIT);
}

int datapoint_to_csv(const struct datapoint *dp, char *buf, size_t buf_size)
{
    if (!dp || !buf) {
        return -EINVAL;
    }

    size_t offset = 0;
    int ret;

    /* 1. timestamp */
    ret = snprintf(buf + offset, buf_size - offset, "%" PRId64 ",", dp->t);
    if (ret < 0 || ret >= (int)(buf_size - offset)) {
        return -ENOMEM;
    }
    offset += ret;

    /* 2. sensor name */
    ret = snprintf(buf + offset, buf_size - offset, "%.*s,", (int)dp->s.len, dp->s.value);
    if (ret < 0 || ret >= (int)(buf_size - offset)) {
        return -ENOMEM;
    }
    offset += ret;

    /* 3. int value (if present) */
    if (dp->n_present) {
        ret = snprintf(buf + offset, buf_size - offset, "%d,", dp->n.n);
    } else {
        ret = snprintf(buf + offset, buf_size - offset, ",");
    }
    if (ret < 0 || ret >= (int)(buf_size - offset)) {
        return -ENOMEM;
    }
    offset += ret;

    /* 4. float value (if present) */
    if (dp->f_present) {
        ret = snprintf(buf + offset, buf_size - offset, "%.6f,", dp->f.f);
    } else {
        ret = snprintf(buf + offset, buf_size - offset, ",");
    }
    if (ret < 0 || ret >= (int)(buf_size - offset)) {
        return -ENOMEM;
    }
    offset += ret;

    /* 5. string value (if present) */
    if (dp->r_present) {
        ret = snprintf(buf + offset, buf_size - offset, "%.*s,", (int)dp->r.r.len, dp->r.r.value);
    } else {
        ret = snprintf(buf + offset, buf_size - offset, ",");
    }
    if (ret < 0 || ret >= (int)(buf_size - offset)) {
        return -ENOMEM;
    }
    offset += ret;

    /* 6. unit string (if present) */
    if (dp->u_present) {
        ret = snprintf(buf + offset, buf_size - offset, "%.*s\n", (int)dp->u.u.len, dp->u.u.value);
    } else {
        ret = snprintf(buf + offset, buf_size - offset, "\n");
    }
    if (ret < 0 || ret >= (int)(buf_size - offset)) {
        return -ENOMEM;
    }
    offset += ret;

    return offset;
}
