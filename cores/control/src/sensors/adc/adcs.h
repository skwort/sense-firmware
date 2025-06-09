#ifndef SENSORS_ADCS_H_
#define SENSORS_ADCS_H_

#include <stdint.h>

#ifdef CONFIG_SENSE_CORE_SENSOR_ADCS

int adcs_init(void);

void adcs_set_poll_interval(int32_t interval_ms);

#endif /* CONFIG_SENSE_CORE_SENSOR_ADCS */

#endif /* SENSORS_ADCS_H_ */
