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
#include <i2c.h>

#if defined(CONFIG_SOC_QUARK_SE_C1000_SS)
#define I2C_DEV CONFIG_I2C_SS_0_NAME
#else
#define I2C_DEV CONFIG_I2C_0_NAME
#endif

/**
 * @file Sample app using the Fujitsu MB85RC256V FRAM through ARC I2C.
 */

void main(void)
{
	struct device *i2c_dev;
	u8_t cmp_data[16];
	u8_t data[16];
	int i, ret;

	i2c_dev = device_get_binding(CONFIG_I2C_FLASH_24FC256_DRV_NAME);
	if(i2c_dev != NULL){
		printf("I2C name is %s\n", CONFIG_I2C_FLASH_24FC256_DRV_NAME);
	}else{
		printf("Device not made\n");
	}
}
