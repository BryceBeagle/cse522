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
#include <sys_clock.h>
#include "main.h"
#include "../../../drivers/flash/i2c_flash_24fc256.h"

#define STACKSIZE 1024

K_THREAD_STACK_DEFINE(stack_area_0, STACKSIZE);
K_THREAD_STACK_DEFINE(stack_area_1, STACKSIZE);

thread_index_card thread_references;
eeprom_buffers measurements;

struct device *EEPROM;
bool is_recording = false;
struct k_sem buffer_write_sem;

eeprom_entry in_buffer[8];
eeprom_entry out_buffer[8];

void radar_read(void *hc_device, void *b, void *c) {

	ARG_UNUSED(c);

	struct device *HCSR = hc_device;

	while (1) {

		u64_t now = tsc_read();

		struct sensor_value measurement;

		sensor_sample_fetch(HCSR);
		sensor_channel_get(HCSR, SENSOR_CHAN_PROX, &measurement);

		u32_t timestamp = (u32_t) measurement.val1;
		u32_t distance = (u32_t) measurement.val2;

		if (is_recording) {
			printk("%i -- t: %i | d: %i\n", *(int *) b, timestamp, distance);
			if (save_distance(timestamp, distance)) {
				// Save distance returns -1 if pages full
				is_recording = false;
			}
		}

		u64_t clocks = tsc_read() - now;
		u64_t completion_time = SYS_CLOCK_HW_CYCLES_TO_NS64(clocks) / 1000000;
		int time_to_sleep = (int) (60 - completion_time);

		if (time_to_sleep < 0) { // timed out, give sensor some time to return
			k_sleep(60);
		} else { // working as intended, sleeping to reach 60ms period length
			k_sleep(time_to_sleep);
		}
	}
}

int save_distance(uint32_t timestamp, uint32_t distance) {

	int ret = 0;

	eeprom_entry measurement = {
			.timestamp = timestamp,
			.distance = distance
	};

	printk("Taking buffer_write semaphore\n");

	// Prevent multiple sensors from modifying buffer at once
	k_sem_take(&buffer_write_sem, K_FOREVER);

	printk("buffer_write semaphore acquired\n");

	// Add new measurement to buffer
	measurements.in_buffer[measurements.num_entries++] = measurement;

	// TODO: Use a const for max
	// We need to send
	if (measurements.num_entries >= 8) {

		printk("Page %zu complete\n", measurements.num_pages);

		// Prevent buffer being changed when at max capacity and eeprom is not
		// Done writing
		k_sem_take(&eeprom_write_sem, K_FOREVER);

		// Move the in_buffer to the out buffer and use the old out_buffer's
		// allocated space as the new in_buffer
		eeprom_entry *temp_buffer = measurements.out_buffer;
		measurements.out_buffer = measurements.in_buffer;
		measurements.in_buffer = temp_buffer;

		// Release semaphore to write data
		k_sem_give(&eeprom_write_sem);

		// Write buffer to eeprom
		printk("Opening thread to write to EEPROM\n");
		int hc_sr_priority = k_thread_priority_get(k_current_get()) + 1;
		k_thread_create(&thread_references.eeprom_writer_thread,
		                stack_area_1, STACKSIZE,
		                write_distance_eeprom, EEPROM,
		                measurements.out_buffer,
		                &(measurements.max_pages),
		                hc_sr_priority, 0, K_NO_WAIT);

		measurements.num_entries = 0;

		if (++measurements.num_pages == measurements.max_pages) {
			// We're done
			ret = 1;
		}
	}

	printk("Giving buffer_write semaphore\n");

	// Allow another thread to touch the buffer
	k_sem_give(&buffer_write_sem);

	return ret;

}

void write_distance_eeprom(void *device,
                           void *buffer, void *offset) {

	struct device *EEPROM = device;
	eeprom_entry *out_buffer = buffer;
	size_t page = *(size_t *) offset;

	k_sem_take(&eeprom_write_sem, K_FOREVER);

	flash_write(EEPROM, page, out_buffer, 8);

	k_sem_give(&eeprom_write_sem);

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

//	// Clear p pages of previous record
//	flash_erase(EEPROM, 0, 512);

	measurements = (eeprom_buffers) {
			.in_buffer = in_buffer,
			.out_buffer = out_buffer,
			.num_entries = 0,
			.num_pages = 0,
			.max_pages = p,
			.is_writing = false
	};

	is_recording = true;

	return 0;
}

int cmd_dump_distances(int argc, char *argv[]) {

	if (argc != 3) return -EINVAL;

	const char *val1 = argv[1], *val2 = argv[2];

	off_t p1 = (off_t) strtol(val1, NULL, 10);
	off_t p2 = (off_t) strtol(val2, NULL, 10);

	int page_buffer[64/sizeof(int)];

	for (off_t i = p1; i <= p2; i++) {
		flash_read(EEPROM, i, page_buffer, sizeof(page_buffer));
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

	EEPROM = device_get_binding(CONFIG_I2C_FLASH_24FC256_DRV_NAME);
	struct device *HCSR_0 = device_get_binding(CONFIG_HC_SR04_0_NAME);
	struct device *HCSR_1 = device_get_binding(CONFIG_HC_SR04_1_NAME);

	printk("EEPROM == null: %i\n", EEPROM == NULL);

	k_sem_init(&eeprom_write_sem, 1, 1);
	k_sem_init(&buffer_write_sem, 1, 1);

	int hc_sr_priority = k_thread_priority_get(k_current_get()) + 1;

	thread_references.hcsr_thread_0_tid =
		k_thread_create(&thread_references.hcsr_thread_0, stack_area_0,
		                STACKSIZE,
	                    radar_read, HCSR_0, (void *) 0, NULL,
	                    hc_sr_priority, 0, K_NO_WAIT);
	thread_references.hcsr_thread_1_tid =
		k_thread_create(&thread_references.hcsr_thread_1, stack_area_1,
		                STACKSIZE,
	                    radar_read, HCSR_1, (void *) 1, NULL,
	                    hc_sr_priority, 0, K_NO_WAIT);

	k_thread_suspend(thread_references.hcsr_thread_0_tid);
	k_thread_suspend(thread_references.hcsr_thread_1_tid);

	// Set up shell
	printk("Hertz %d\n", sys_clock_hw_cycles_per_sec);
	init_shell();
}
