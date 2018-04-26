/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <flash.h>
#include <device.h>
#include <sensor.h>
#include <console.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <shell/shell.h>
#include <gpio.h>
#include <board.h>

#define STACKSIZE 1024

K_THREAD_STACK_DEFINE(stack_area_0, STACKSIZE);
K_THREAD_STACK_DEFINE(stack_area_1, STACKSIZE);

// TODO: somehow not global?
k_tid_t hcsr_thread_0_tid, hcsr_thread_1_tid;
struct device *EEPROM_0;

enum pin_level {
	PIN_LOW = 0x00,
	PIN_HIGH = 0x01,
	DONT_CARE = 0xFF,
};

void radar_read(void *hc_device, void *b, void *c) {

	ARG_UNUSED(b);
	ARG_UNUSED(c);

	printk("Thread running\n");

	struct device *HCSR = hc_device;

	while (1) {

		struct sensor_value distance;

		printk("Fetching\n");
		sensor_sample_fetch(HCSR);

		printk("Getting distance\n");
		sensor_channel_get(HCSR, SENSOR_CHAN_PROX, &distance);

		printk("Distance: %d.%06d\n", distance.val1, distance.val2);

		k_sleep(1000);

	}
}

int cmd_enable_distance_sensors(int argc, char *argv[]) {

	if (argc != 2) return -EINVAL;

	const char *val = argv[1];

	if (!strcmp(val, "0")) {
		printk("Disabling both sensors\n");
		k_thread_suspend(hcsr_thread_0_tid);
		k_thread_suspend(hcsr_thread_1_tid);
	} else if (!strcmp(val, "1")) {
		printk("Enabling HCSR0\n");
		k_thread_resume(hcsr_thread_0_tid);
		k_thread_suspend(hcsr_thread_1_tid);
	} else if (!strcmp(val, "2")) {
		printk("Enabling HCSR1\n");
		k_thread_resume(hcsr_thread_0_tid);
		k_thread_resume(hcsr_thread_1_tid);
	}

	return 0;

}

int cmd_start_recording(int argc, char *argv[]) {

	if (argc != 2) return -EINVAL;

	const char *val = argv[1];

	size_t p = (size_t) strtol(val, NULL, 10);

	flash_erase(EEPROM_0, NULL, p);

	return 0;
}

int cmd_dump_distances(int argc, char *argv[]) {

	if (argc != 3) return -EINVAL;

	const char *val1 = argv[1], *val2 = argv[2];

	size_t p1 = (size_t) strtol(val1, NULL, 10);
	size_t p2 = (size_t) strtol(val2, NULL, 10);

	int page_buffer[1024];

	for (off_t i = p1; i <= p2; i++) {
		flash_read(EEPROM_0, p1, page_buffer, sizeof(page_buffer));
		for (int x = 0; x < 1024; x++) {
			printk("%d\n", page_buffer[x]);
		}
	}

	return 0;

}

static const struct shell_cmd commands[] = {
		{"Enable", cmd_enable_distance_sensors},
		{"Start",  cmd_start_recording},
		{"Dump",   cmd_dump_distances},
		{NULL, NULL}
};

void init_shell() {

	SHELL_REGISTER("assign4", commands);
	shell_register_default_module("assign4");
	shell_init("> ");
}

static void hc_sr04_gpio_callback(struct device *dev,
                                  struct gpio_callback *cb, u32_t pins) {
	ARG_UNUSED(pins);

	printk("    Callback called\n");

	printk("    CB Reading TSC\n");
	u64_t now = _tsc_read();
//
//	printk("Getting container\n");
//	struct hc_sr04_data *drv_data = CONTAINER_OF(cb,
//	                                             struct hc_sr04_data,
//	                                             gpio_cb);
//
//	printk("Getting distance\n");
//	drv_data->distance = (now - drv_data->start_time) * 1000000
//	                     / 400000000 / 58;
//
//	printk("    CB Giving semaphore\n");
//	k_sem_give(&drv_data->data_sem);
}

void main(void) {

	struct k_thread hcsr_thread_0, hcsr_thread_1;

	EEPROM_0 = device_get_binding(PINMUX_GALILEO_EXP1_NAME);
	struct device *HCSR_0 = device_get_binding(CONFIG_HC_SR04_NAME);
	struct device *HCSR_1 = device_get_binding(CONFIG_HC_SR04_NAME);

	struct device *gpio = device_get_binding(PINMUX_GALILEO_GPIO_DW_NAME);
	struct device *exp = device_get_binding(PINMUX_GALILEO_GPIO_DW_NAME);

	struct gpio_callback gpio_cb;
	gpio_init_callback(&gpio_cb, hc_sr04_gpio_callback,
	                   BIT(CONFIG_HC_SR04_GPIO_PIN_NUM_1));
	if (gpio_add_callback(gpio, &gpio_cb) < 0) {
		printk("Failed to set GPIO callback\n");
		return;
	}

	int res;

	/* setup data ready gpio interrupt on shield pin 0*/
	res = gpio_pin_configure(exp,
	                         CONFIG_HC_SR04_EXP_PIN_NUM_1, GPIO_DIR_OUT);
	if (res) printk("Error 7\n");
	res = gpio_pin_configure(exp,
	                         CONFIG_HC_SR04_EXP_PIN_NUM_2, GPIO_DIR_OUT);
	if (res) printk("Error 8\n");
	res = gpio_pin_write(exp,
	                     CONFIG_HC_SR04_EXP_PIN_NUM_1, PIN_HIGH);
	if (res) printk("Error 9\n");
	res = gpio_pin_write(exp,
	                     CONFIG_HC_SR04_EXP_PIN_NUM_2, PIN_LOW);
	if (res) printk("Error 10\n");

	res = gpio_pin_configure(gpio, CONFIG_HC_SR04_GPIO_PIN_NUM_1,
	                         GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
	                         GPIO_INT_ACTIVE_HIGH);
	if (res) printk("Error 11\n");

	gpio_pin_enable_callback(gpio, CONFIG_HC_SR04_GPIO_PIN_NUM_1);

	int priority = k_thread_priority_get(k_current_get()) + 1;

	printk("Creating threads with priority %i\n", priority);
	hcsr_thread_0_tid = k_thread_create(&hcsr_thread_0, stack_area_0, STACKSIZE,
	                                    radar_read, HCSR_0, NULL, NULL,
	                                    priority, 0, K_NO_WAIT);
	hcsr_thread_1_tid = k_thread_create(&hcsr_thread_1, stack_area_1, STACKSIZE,
	                                    radar_read, HCSR_1, NULL, NULL,
	                                    priority, 0, K_NO_WAIT);

	printk("Threads created\n");

	k_thread_suspend(hcsr_thread_0_tid);
	k_thread_suspend(hcsr_thread_1_tid);

	// Set up shell
	init_shell();

	for (;;) {
		k_sleep(10);
	}

	// TODO: Yield?
}
