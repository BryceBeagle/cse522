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

static void hc_sr04_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	u64_t now = tsc_read();
	struct hc_sr04_data *drv_data =
		CONTAINER_OF(cb, struct hc_sr04_data, gpio_cb);

	ARG_UNUSED(pins);

	drv_data->distance = (now - drv_data->distance) * 1000000 / 400000000 / 58;

	k_sem_give(&drv_data->data_sem);
}

static int hc_sr04_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct hc_sr04_data *drv_data = dev->driver_data;

	k_sem_take(&drv_data->data_sem, K_FOREVER);

	drv_data->distance = tsc_read();
	gpio_pin_write(drv_data->gpio, CONFIG_HC_SR04_GPIO_PIN_NUM_2, PIN_HIGH);
	k_busy_wait(10);
	gpio_pin_write(drv_data->gpio, CONFIG_HC_SR04_GPIO_PIN_NUM_2, PIN_LOW);

	k_sem_take(&drv_data->data_sem, K_FOREVER);

	return 0;
}


static int hc_sr04_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct hc_sr04_data *drv_data = dev->driver_data;

	if (chan == SENSOR_CHAN_PROX) {
		val->val1 = drv_data->distance;
		val->val2 = 0;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api hc_sr04_driver_api = {
	.sample_fetch = hc_sr04_sample_fetch,
	.channel_get = hc_sr04_channel_get,
};

static int hc_sr04_init(struct device *dev)
{
	struct hc_sr04_data *drv_data = dev->driver_data;

	drv_data->exp  = device_get_binding(CONFIG_HC_SR04_EXP_DEV_NAME);
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

	k_sem_init(&drv_data->data_sem, 0, 1);

	/* setup data ready gpio trigger on shield pin 2*/
	gpio_pin_configure(drv_data->exp, CONFIG_HC_SR04_EXP_PIN_NUM_3, GPIO_DIR_OUT);
	gpio_pin_write(drv_data->exp, CONFIG_HC_SR04_EXP_PIN_NUM_3, PIN_LOW);
	gpio_pin_configure(drv_data->exp, CONFIG_HC_SR04_EXP_PIN_NUM_4, GPIO_DIR_OUT);
	gpio_pin_write(drv_data->exp, CONFIG_HC_SR04_EXP_PIN_NUM_4, PIN_LOW);
	gpio_pin_configure(drv_data->gpio, CONFIG_HC_SR04_GPIO_PIN_NUM_2, GPIO_DIR_OUT);
	gpio_pin_write(drv_data->gpio, CONFIG_HC_SR04_GPIO_PIN_NUM_2, PIN_LOW);

	/* setup data ready gpio interrupt on shield pin 0*/
	gpio_pin_configure(drv_data->exp, CONFIG_HC_SR04_EXP_PIN_NUM_1, GPIO_DIR_OUT);
	gpio_pin_write(drv_data->exp, CONFIG_HC_SR04_EXP_PIN_NUM_1, PIN_HIGH);
	gpio_pin_configure(drv_data->exp, CONFIG_HC_SR04_EXP_PIN_NUM_2, GPIO_DIR_OUT);
	gpio_pin_write(drv_data->exp, CONFIG_HC_SR04_EXP_PIN_NUM_2, PIN_LOW);

	gpio_pin_configure(drv_data->gpio, CONFIG_HC_SR04_GPIO_PIN_NUM_1,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH);
	gpio_init_callback(&drv_data->gpio_cb,
			   hc_sr04_gpio_callback,
			   BIT(CONFIG_HC_SR04_GPIO_PIN_NUM_1));
	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		SYS_LOG_DBG("Failed to set GPIO callback");
		return -EIO;
	}

	dev->driver_api = &hc_sr04_driver_api;

	return 0;
}

static struct hc_sr04_data hc_sr04_data;

DEVICE_INIT(hc_sr04, CONFIG_HC_SR04_NAME, hc_sr04_init, &hc_sr04_data,
	    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);
