#ifndef DATAPOINT_HELPER_H_
#define DATAPOINT_HELPER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

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

#endif /* DATAPOINT_HELPERS_H_ */
