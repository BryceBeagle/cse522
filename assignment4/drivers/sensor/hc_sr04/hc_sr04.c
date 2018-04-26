/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <gpio.h>
#include <kernel.h>
#include <sensor.h>
#include <misc/util.h>
#include <misc/__assert.h>

#include "hc_sr04.h"

enum pin_level {
	PIN_LOW = 0x00,
	PIN_HIGH = 0x01,
	DONT_CARE = 0xFF,
};

#define tsc_read() (_tsc_read())

static int hc_sr04_sample_fetch(struct device *dev, enum sensor_channel chan) {
	struct hc_sr04_data *drv_data = dev->driver_data;

	printk("    Taking semaphore\n");
	k_sem_take(&drv_data->data_sem, K_FOREVER);

	printk("    Reading TSC\n");
	drv_data->start_time = tsc_read();

	printk("    Setting pin high\n");
	gpio_pin_write(drv_data->gpio, CONFIG_HC_SR04_GPIO_PIN_NUM_2, PIN_HIGH);

	printk("    Waiting for 10 us\n");
	k_busy_wait(10);

	printk("    Setting pin low\n");
	gpio_pin_write(drv_data->gpio, CONFIG_HC_SR04_GPIO_PIN_NUM_2, PIN_LOW);

	printk("    Taking semaphore again\n");
	k_sem_take(&drv_data->data_sem, K_FOREVER);

	printk("    Giving semaphore");
	k_sem_give(&drv_data->data_sem);

	printk("Returning\n");
	return 0;
}


static int hc_sr04_channel_get(struct device *dev,
                               enum sensor_channel chan,
                               struct sensor_value *val) {
	struct hc_sr04_data *drv_data = dev->driver_data;

	if (chan == SENSOR_CHAN_PROX) {
		val->val1 = (s32_t) drv_data->distance;
		val->val2 = (s32_t) ((drv_data->distance - val->val1) * 1E6);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api hc_sr04_driver_api = {
		.sample_fetch = hc_sr04_sample_fetch,
		.channel_get  = hc_sr04_channel_get,
};

static int hc_sr04_init(struct device *dev) {
	struct hc_sr04_data *drv_data = dev->driver_data;

	drv_data->exp = device_get_binding(CONFIG_HC_SR04_EXP_DEV_NAME);
	drv_data->gpio = device_get_binding(CONFIG_HC_SR04_GPIO_DEV_NAME);
	if (drv_data->exp == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device",
		            CONFIG_HC_SR04_EXP_DEV_NAME);
		return -EINVAL;
	}
	if (drv_data->gpio == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device",
		            CONFIG_HC_SR04_GPIO_DEV_NAME);
		return -EINVAL;
	}

	k_sem_init(&drv_data->data_sem, 1, 1);

	int res;

	/* setup data ready gpio trigger on shield pin 2*/
	res = gpio_pin_configure(drv_data->exp,
	                         CONFIG_HC_SR04_EXP_PIN_NUM_3, GPIO_DIR_OUT);
	if (res) printk("Error 1\n");
	res = gpio_pin_configure(drv_data->exp,
	                         CONFIG_HC_SR04_EXP_PIN_NUM_4, GPIO_DIR_OUT);
	if (res) printk("Error 2\n");
	res = gpio_pin_configure(drv_data->gpio,
	                         CONFIG_HC_SR04_GPIO_PIN_NUM_2, GPIO_DIR_OUT);
	if (res) printk("Error 3\n");

	res = gpio_pin_write(drv_data->exp,
	                     CONFIG_HC_SR04_EXP_PIN_NUM_3, PIN_LOW);
	if (res) printk("Error 4\n");
	res = gpio_pin_write(drv_data->exp,
	                     CONFIG_HC_SR04_EXP_PIN_NUM_4, PIN_LOW);
	if (res) printk("Error 5\n");
	res = gpio_pin_write(drv_data->gpio,
	                     CONFIG_HC_SR04_GPIO_PIN_NUM_2, PIN_LOW);
	if (res) printk("Error 6\n");

	/* setup data ready gpio interrupt on shield pin 0*/
	res = gpio_pin_configure(drv_data->exp,
	                         CONFIG_HC_SR04_EXP_PIN_NUM_1, GPIO_DIR_OUT);
	if (res) printk("Error 7\n");
	res = gpio_pin_configure(drv_data->exp,
	                         CONFIG_HC_SR04_EXP_PIN_NUM_2, GPIO_DIR_OUT);
	if (res) printk("Error 8\n");
	res = gpio_pin_write(drv_data->exp,
	                     CONFIG_HC_SR04_EXP_PIN_NUM_1, PIN_HIGH);
	if (res) printk("Error 9\n");
	res = gpio_pin_write(drv_data->exp,
	                     CONFIG_HC_SR04_EXP_PIN_NUM_2, PIN_LOW);
	if (res) printk("Error 10\n");

	res = gpio_pin_configure(drv_data->gpio, CONFIG_HC_SR04_GPIO_PIN_NUM_1,
	                         GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
	                         GPIO_INT_ACTIVE_HIGH);
	if (res) printk("Error 11\n");

	dev->driver_api = &hc_sr04_driver_api;

	return 0;
}

static struct hc_sr04_data hc_sr04_data;

DEVICE_INIT(hc_sr04, CONFIG_HC_SR04_NAME, hc_sr04_init, &hc_sr04_data,
            NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);
