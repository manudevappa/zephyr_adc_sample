/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <nrf.h>
#include <stdio.h>
#include <string.h>
#include <drivers/uart.h>
#include <drivers/adc.h>
#include <zephyr.h>

struct device *adc_dev;

#include <hal/nrf_saadc.h>
#define ADC_DEVICE_NAME "ADC_0"						// Device tree label name to get device structure
#define ADC_RESOLUTION 10							// Use 1024 points to map complete ADC values
#define ADC_GAIN ADC_GAIN_1_6						// Devide the incoming voltage by 6 and pass to ADC circuit
#define ADC_REFERENCE ADC_REF_INTERNAL				// 0.6V 
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_1ST_CHANNEL_ID 0
#define ADC_1ST_CHANNEL_INPUT NRF_SAADC_INPUT_AIN0 // N channel Input not configured now
#define ADC_2ND_CHANNEL_ID 1
#define ADC_2ND_CHANNEL_INPUT NRF_SAADC_INPUT_AIN1	// N channel Input not configured now
#define ADC_3RD_CHANNEL_ID 2
#define ADC_3RD_CHANNEL_INPUT NRF_SAADC_INPUT_AIN2	// N channel Input not configured now
#define ADC_4TH_CHANNEL_ID 3
#define ADC_4TH_CHANNEL_INPUT NRF_SAADC_INPUT_AIN3	// N channel Input not configured now

#define BAT_INPUT_VOLT_RANGE     3.6f  			// Max voltage we can pass to ADC inputs (0.6/(1/6) = 3.6v)
#define BAT_VALUE_RANGE_10_BIT   1023.0f 		// (2^10 - 1)


static const struct adc_channel_cfg m_4th_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_4TH_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_4TH_CHANNEL_INPUT,
#endif
};

static const struct adc_channel_cfg m_3rd_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_3RD_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_3RD_CHANNEL_INPUT,
#endif
};

static const struct adc_channel_cfg m_2nd_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_2ND_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_2ND_CHANNEL_INPUT,
#endif
};

static const struct adc_channel_cfg m_1st_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_1ST_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_1ST_CHANNEL_INPUT,
#endif
};

#define BUFFER_SIZE 1 * 5
static signed short m_sample_buffer[BUFFER_SIZE];

const struct adc_sequence sequence = {
	.channels = BIT(ADC_1ST_CHANNEL_ID) | BIT(ADC_2ND_CHANNEL_ID) |
			BIT(ADC_3RD_CHANNEL_ID) | BIT(ADC_4TH_CHANNEL_ID),
	.buffer = m_sample_buffer,
	.buffer_size = sizeof(m_sample_buffer),
	.resolution = ADC_RESOLUTION,
};

static int adc_sample(void)
{
	int ret;
	if (!adc_dev) {
		return -1;
	}
	ret = adc_read(adc_dev, &sequence);
	if(ret>0){
		printk("ADC read err: %d\n", ret);
	}

	/* Print the AIN0 values */
	for (int i = 0; i < 4; i++) {
		printk("Raw %d : %d \t",i, m_sample_buffer[i]);
		float adc_voltage = m_sample_buffer[i] * (BAT_INPUT_VOLT_RANGE / BAT_VALUE_RANGE_10_BIT);
		printf("CH %d: ADC %f \n", i, adc_voltage);
		m_sample_buffer[i] = 0;
	}
	printf("\n");
	return ret;
}

int main(void)
{
	int err;

	adc_dev = device_get_binding(ADC_DEVICE_NAME);
	if (!adc_dev) {
		printk("device_get_binding ADC_0 failed\n");
	}
	err = adc_channel_setup(adc_dev, &m_1st_channel_cfg);
	if (err) {
		printk("Error in adc setup: %d\n", err);
	}
	err = adc_channel_setup(adc_dev, &m_2nd_channel_cfg);
	if (err) {
		printk("Error in adc setup: %d\n", err);
	}

	err = adc_channel_setup(adc_dev, &m_3rd_channel_cfg);
	if (err) {
		printk("Error in adc setup: %d\n", err);
	}

	err = adc_channel_setup(adc_dev, &m_4th_channel_cfg);
	if (err) {
		printk("Error in adc setup: %d\n", err);
	}

	/* Trigger offset calibration
	 * As this generates a _DONE and _RESULT event
	 * the first result will be incorrect.
	 */
	NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
	while (1) {
		err = adc_sample();
		if (err) {
			printk("Error in adc sampling: %d\n", err);
		}
		k_sleep(K_MSEC(1000));
	}
}