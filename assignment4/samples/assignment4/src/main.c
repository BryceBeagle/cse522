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

#define STACKSIZE 1024

K_THREAD_STACK_DEFINE(stack_area_0, STACKSIZE);
K_THREAD_STACK_DEFINE(stack_area_1, STACKSIZE);

void radar_read(void *hc_device, void *b, void *c) {
	printk("Thread running");

	ARG_UNUSED(b);
	ARG_UNUSED(c);

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

void main(void) {

	printk("Main starting\n");

	struct k_thread hcsr_thread_0, hcsr_thread_1;
	k_tid_t hcsr_thread_0_tid, hcsr_thread_1_tid;

	struct device *EEPROM_0 = device_get_binding(CONFIG_I2C_FLASH_24FC256_DRV_NAME);
	struct device *HCSR_0 = device_get_binding(CONFIG_HC_SR04_NAME);
//	struct device *HCSR_1 = device_get_binding(CONFIG_HC_SR04_NAME);

	printk("Creating thread\n");
	hcsr_thread_0_tid = k_thread_create(&hcsr_thread_0, stack_area_0, STACKSIZE,
	                                    radar_read, HCSR_0, NULL, NULL,
	                                    k_thread_priority_get(k_current_get()) + 1, 0, K_NO_WAIT);

	printk("Thread created\n");

//	hcsr_thread_1_tid = k_thread_create(&hcsr_thread_1, stack_area_1, STACKSIZE,
//	                                    radar_read, HCSR_1, NULL, NULL,
//	                                    k_thread_priority_get(k_current_get()) + 1, 0, K_NO_WAIT);
//
//	//perhaps set time slicing for radar tasks
//
//	k_thread_suspend(hcsr_thread_0_tid);
//	k_thread_suspend(hcsr_thread_1_tid);
//
//	console_getline_init();
//	while (1) {
//		char *command_line = console_getline();
//		char *command_type = strtok(command_line, " ");
//		if (strcmp(command_type, "Enable")) {
//			//enable the two threads running the sensors
//			char *input_parameter = strtok(NULL, " ");
//			char n = strtol(p_string, NULL, 10);
//
//			switch (n) {
//				case 2:
//					//enable HCSR_0's thread
//					//enable HCSR_1's thread
//					k_thread_resume(hcsr_thread_0_tid);
//					k_thread_resume(hcsr_thread_1_tid);
//					break;
//				case 1:
//					//enable HCSR_0's thread
//					//disable HCSR_1's thread
//					k_thread_resume(hcsr_thread_0_tid);
//					k_thread_suspend(hcsr_thread_1_tid);
//					break;
//				case 0:
//					//disable HCSR_0's thread
//					//disable HCSR_1's thread
//					k_thread_suspend(hcsr_thread_0_tid);
//					k_thread_suspend(hcsr_thread_1_tid);
//					break;
//				default:
//					printk("Exiting\n");
//					exit(-1);
//			}
//
//		} else if (strcmp(command_type, "Start")) {
//			//erase the pages up to input p
//			char *p_string = strtok(NULL, " ");
//			size_t p = strtol(p_string, NULL, 10);
//
//			flash_erase(EEPROM_0, NULL, p);
//
//		} else if (strcmp(command_type, "Dump")) {
//			//print pages between input p1 and p2
//			char *p1_string = strtok(NULL, " ");
//			off_t p1 = strtol(p1_string, NULL, 10);
//			char *p2_string = strtok(NULL, " ");
//			off_t p2 = strtol(p2_string, NULL, 10);
//			int page_buffer[1024];
//
//			for (off_t i = p1; i <= p2; i++) {
//				flash_read(EEPROM_0, p1, page_buffer, sizeof(page_buffer));
//				for (int x = 0; x < 1024; x++) {
//					printk("%d\n", page_buffer[x]);
//				}
//			}
//
//		}
//
//		//yield to the threads in some fashion
//		NULL;
//	}
}
