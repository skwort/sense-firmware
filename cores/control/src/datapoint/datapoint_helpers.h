#ifndef DATAPOINT_HELPER_H_
#define DATAPOINT_HELPER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include <datapoint/czd_datapoint_types.h>

int get_and_submit_sensor_datapoint(const struct device *dev,
                                    enum sensor_channel chan,
                                    const char *label,
                                    const char *unit);

int submit_float_datapoint(double value,
                           const char *label,
                           const char *unit);

int submit_int_datapoint(int value,
                         const char *label,
                         const char *unit);

int datapoint_to_csv(const struct datapoint *dp, char *buf, size_t buf_size);

#endif /* DATAPOINT_HELPERS_H_ */
