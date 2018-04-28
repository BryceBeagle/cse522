/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <flash.h>
#include <sensor.h>
#include <stdlib.h>
#include <string.h>
#include <shell/shell.h>
#include <gpio.h>
#include "main.h"

#define STACKSIZE 1024

K_THREAD_STACK_DEFINE(stack_area_0, STACKSIZE);
K_THREAD_STACK_DEFINE(stack_area_1, STACKSIZE);

thread_index_card thread_references;
struct device *EEPROM_0;

void radar_read(void *hc_device, void *b, void *c) {

	ARG_UNUSED(c);

	struct device *HCSR = hc_device;

	while (1) {
		u64_t now = tsc_read();

		struct sensor_value distance;

		sensor_sample_fetch(HCSR);

		sensor_channel_get(HCSR, SENSOR_CHAN_PROX, &distance);

		printk("%d -- Distance: %d.%06d\n", (int)b, distance.val1, distance.val2);

		k_sleep((20000000 - (now - tsc_read())) * 3 / 1000000);
	}
}

int cmd_enable_distance_sensors(int argc, char *argv[]) {

	if (argc != 2) return -EINVAL;

	const char *val = argv[1];

	if (!strcmp(val, "0")) {
		printk("Disabling both sensors\n");
		k_thread_suspend(thread_references.hcsr_thread_0_tid);
		k_thread_suspend(thread_references.hcsr_thread_1_tid);
	} else if (!strcmp(val, "1")) {
		printk("Enabling HCSR0\n");
		k_thread_resume(thread_references.hcsr_thread_0_tid);
		k_thread_suspend(thread_references.hcsr_thread_1_tid);
	} else if (!strcmp(val, "2")) {
		printk("Enabling HCSR1\n");
		k_thread_resume(thread_references.hcsr_thread_0_tid);
		k_thread_resume(thread_references.hcsr_thread_1_tid);
	}

	return 0;

}

int cmd_start_recording(int argc, char *argv[]) {

	if (argc != 2) return -EINVAL;

	const char *val = argv[1];

	size_t p = (size_t) strtol(val, NULL, 10);

	flash_erase(EEPROM_0, 0, p);

	return 0;
}

int cmd_dump_distances(int argc, char *argv[]) {

	if (argc != 3) return -EINVAL;

	const char *val1 = argv[1], *val2 = argv[2];

	off_t p1 = (off_t) strtol(val1, NULL, 10);
	off_t p2 = (off_t) strtol(val2, NULL, 10);

	int page_buffer[64/sizeof(int)];

	for (off_t i = p1; i <= p2; i++) {
		flash_read(EEPROM_0, i, page_buffer, sizeof(page_buffer));
		for (int x = 0; x < 64/sizeof(int); x++) {
			printk("%d\n", page_buffer[x]);
		}
	}

	return 0;

}

void init_shell() {
	SHELL_REGISTER("assign4", commands);
	shell_register_default_module("assign4");
	shell_init("> ");
}

void main(void) {

	EEPROM_0 = device_get_binding(CONFIG_I2C_FLASH_24FC256_DRV_NAME);
	struct device *HCSR_0 = device_get_binding(CONFIG_HC_SR04_0_NAME);
	struct device *HCSR_1 = device_get_binding(CONFIG_HC_SR04_1_NAME);

	int hc_sr_priority = k_thread_priority_get(k_current_get()) + 1;

	thread_references.hcsr_thread_0_tid =
		k_thread_create(&thread_references.hcsr_thread_0, stack_area_0, STACKSIZE,
	                        radar_read, HCSR_0, (void *)0, NULL,
	                        hc_sr_priority, 0, K_NO_WAIT);
	thread_references.hcsr_thread_1_tid =
		k_thread_create(&thread_references.hcsr_thread_1, stack_area_1, STACKSIZE,
	                        radar_read, HCSR_1, (void *)1, NULL,
	                        hc_sr_priority, 0, K_NO_WAIT);

	k_thread_suspend(thread_references.hcsr_thread_0_tid);
	k_thread_suspend(thread_references.hcsr_thread_1_tid);

	// Set up shell
	init_shell();
}
