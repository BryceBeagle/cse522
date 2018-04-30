/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SENSOR_HC_SR04
#define _SENSOR_HC_SR04

#include <kernel.h>

struct hc_sr04_data {
	struct device *gpio;
	struct gpio_callback gpio_cb;
	struct k_sem data_sem;

	uint32_t trig_gpio_pin;

	u64_t timestamp;
	u64_t distance;

};

#include <logging/sys_log.h>
#endif
