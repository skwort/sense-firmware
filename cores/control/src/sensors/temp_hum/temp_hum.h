#ifndef SENSORS_TEMP_HUM_H_
#define SENSORS_TEMP_HUM_H_

#include <stdint.h>

#ifdef CONFIG_SENSE_CORE_SENSOR_SHT4X

int sht4x_init(void);

void sht4x_set_poll_interval(int32_t interval_ms);

#endif /* CONFIG_SENSE_CORE_SENSOR_SHT4X */

#endif /* SENSORS_TEMP_HUM_H_ */
