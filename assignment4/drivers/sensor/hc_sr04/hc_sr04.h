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
	struct device *exp;
	struct gpio_callback gpio_cb;

	struct k_sem data_sem;

	u64_t start_time;
	float distance;
};

#define SYS_LOG_DOMAIN "HC_SR04"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>
#endif
