#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>

LOG_MODULE_REGISTER(adcs, LOG_LEVEL_INF);

/* ADC nodes from devicetree. */
#define ADC0_NODE DT_NODELABEL(ads1015_adc0)
#define ADC1_NODE DT_NODELABEL(ads1015_adc1)

#if !(DT_NODE_HAS_STATUS_OKAY(ADC0_NODE) && \
      DT_NODE_HAS_STATUS_OKAY(ADC1_NODE))
#error "Both ADC0 and ADC1 nodes must be enabled in the devicetree."
#endif

#define ADC_SE_CHANS 4

/* ADC sequence resolutions.
 * ads1015 is 12bit, but driver requires SE resolution to be 11. */
#define ADC_SEQUENCE_RESOLUTION 12
#define ADC_SEQUENCE_RESOLUTION_SE 11

const struct device *adc0 = DEVICE_DT_GET(ADC0_NODE);
const struct device *adc1 = DEVICE_DT_GET(ADC1_NODE);

static struct k_work_delayable adcs_poll_work;
static atomic_t poll_interval_ms = 500;

void poll_ads1015(const struct device *adc)
{
    int err;
	uint16_t channel_reading[CONFIG_SENSE_CORE_SENSOR_ADCS_SEQ_SAMPLES];

	/* Options for the sequence sampling. */
	const struct adc_sequence_options options = {
		.extra_samplings = CONFIG_SENSE_CORE_SENSOR_ADCS_SEQ_SAMPLES - 1,
		.interval_us = 0,
	};

	/* Configure the sampling sequence to be made. */
	struct adc_sequence sequence = {
		.buffer = channel_reading,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(channel_reading),
		.resolution = ADC_SEQUENCE_RESOLUTION_SE,
		.options = &options,
        .channels = BIT(0),     // channel bitmask, only channel 0 supported
        .oversampling = 0,      // oversampling not supported.
	};

    uint8_t adc_channel_bitmask = 0b1;

    for (int chan = 0; chan < ADC_SE_CHANS; chan++) {

        if (~adc_channel_bitmask & (1 << chan)) {
            continue;
        }

        struct adc_channel_cfg adc_cfg;

        /* Config struct for adc. Note that despite having 4 apparent channels,
         * the driver expects single channel operation, and the vref to be the
         * internal refernce. */
        adc_cfg.channel_id = 0;
        adc_cfg.reference = ADC_REF_INTERNAL;

        /* Configuration of the multiplexer is done using the input_postive and
         * input_negative struct members. Refer to the driver code for more
         * details. The idea behind sampling each channel is to set the channel
         * config, sample, update the channel config, then sample again, and so
         * on for each channel. Note that since we are operating in
         * single-ended mode, the input_negative channel is effectively
         * ignored. */
        adc_cfg.differential = 0;
        adc_cfg.input_positive = chan;
        adc_cfg.input_negative = 0;

        /* Here we set the gain to 1/3. Refer to the datasheet and driver for
        * available options. */
        adc_cfg.gain = ADC_GAIN_1_3;

        /* Here we set the acquisition time to default */
        adc_cfg.acquisition_time = ADC_ACQ_TIME_DEFAULT;

        err = adc_channel_setup(adc, &adc_cfg);
        if (err < 0) {
            LOG_ERR("Could not setup %s chan %d (%d)\n", adc->name, chan, err);
            return;
        }

        /* Get the reference voltage from the adc */
        uint32_t vrefs_mv = adc_ref_internal(adc);
        if (vrefs_mv == 0) {
            LOG_WRN("ADC internal reference not available.");
            return;
        }

        err = adc_read(adc, &sequence);
		if (err < 0) {
			LOG_ERR("Could not read adc (%d)\n", err);
            return;
		}

        LOG_INF("%s: channel %d", adc->name, chan);
        int32_t val_mv[CONFIG_SENSE_CORE_SENSOR_ADCS_SEQ_SAMPLES];
        for (size_t idx = 0U; idx < CONFIG_SENSE_CORE_SENSOR_ADCS_SEQ_SAMPLES; idx++) {
            val_mv[idx] = channel_reading[idx];
            err = adc_raw_to_millivolts(vrefs_mv,
                                        adc_cfg.gain,
                                        ADC_SEQUENCE_RESOLUTION_SE,
                                        &val_mv[idx]);
            LOG_INF("  Sequence[%d]: %" PRIu32 " mv", idx, val_mv[idx]);
        }
    }
}

void adcs_poll_work_handler(struct k_work *work)
{
    poll_ads1015(adc0);
    poll_ads1015(adc1);

    k_work_schedule(&adcs_poll_work, K_MSEC(atomic_get(&poll_interval_ms)));
}

int adcs_init(void)
{
    if (!device_is_ready(adc0)) {
		LOG_ERR("Device %s is not ready.", adc0->name);
        return 1;
    }

    if (!device_is_ready(adc1)) {
		LOG_ERR("Device %s is not ready.", adc1->name);
        return 1;
    }

    k_work_init_delayable(&adcs_poll_work, adcs_poll_work_handler);
    k_work_schedule(&adcs_poll_work, K_NO_WAIT);

    return 0;
}

void adcs_set_poll_interval(int32_t interval_ms)
{
    atomic_set(&poll_interval_ms, interval_ms);
    k_work_reschedule(&adcs_poll_work, K_MSEC(interval_ms));
}
