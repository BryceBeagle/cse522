/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <flash.h>
#include <i2c.h>
#include <init.h>
#include <string.h>
#include "i2c_flash_24fc256.h"
//view i2c_flash_w25qxxdv.c for reference
//must implement checks and retrys for when device is busy

static int i2c_flash_wb_read(struct device *dev, off_t offset, void *data,
			     size_t len)
{
	// random sequential read
	u8_t read_control_buffer[2];
	int ret = 0;
	k_sem_take(&driver_data->sem, K_FOREVER);

	read_control_buffer[0] = (offset & 0xFF00) >> 8;
	read_control_buffer[1] = (offset & 0x00FF) >> 0;
	read_data[0] = 0xA1;

	ret |= i2c_write(driver_data->i2c, read_control_buffer, 2, 0xA0);
	ret |= i2c_read(driver_data->i2c, data, len, 0xA1);

	k_sem_give(&driver_data->sem);
	return ret;
}

static int i2c_flash_wb_write(struct device *dev, off_t offset,
			      const void *data, size_t len)
{
	// only allow page writes (64 bytes)
	u8_t buffer[67];
	int ret = 0;
	k_sem_take(&driver_data->sem, K_FOREVER);

	if(len != 64){
		ret = -1;
	}else{
		buffer[0] = 0xA0;
		buffer[1] = (offset & 0xFF00) >> 8;
		buffer[2] = (offset & 0x00FF) >> 0;
		memcpy(buffer+3, data, len);

		ret |= i2c_write(driver_data->i2c, buffer, 0xA0);
	}

	k_sem_give(&driver_data->sem);
	return ret;
}

static int i2c_flash_wb_write_protection_set(struct device *dev, bool enable)
{
	// toggles WP pin to 'enable'
	// no need to implement
	k_sem_take(&driver_data->sem, K_FOREVER);
	k_sem_give(&driver_data->sem);
	return 0;
}

static int i2c_flash_wb_erase(struct device *dev, off_t offset, size_t size)
{
	// call write with an array of zeros for every page
	k_sem_take(&driver_data->sem, K_FOREVER);

	k_sem_give(&driver_data->sem);

	return 0;
}

static const struct flash_driver_api i2c_flash_api = {
	.read = i2c_flash_wb_read,
	.write = i2c_flash_wb_write,
	.erase = i2c_flash_wb_erase,
	.write_protection = i2c_flash_wb_write_protection_set,
	.write_block_size = 1,
};

static int i2c_flash_init(struct device *dev)
{
	struct device *i2c_dev;
	struct i2c_flash_data *data = dev->driver_data;
	int ret;

	i2c_dev = device_get_binding(CONFIG_I2C_FLASH_24FC256_I2C_NAME);
	if (!i2c_dev) {
		return -EIO;
	}

	data->i2c = i2c_dev;

	k_sem_init(&data->sem, 1, UINT_MAX);

	ret = i2c_configure(dev, 0);
	if (!ret) {
		dev->driver_api = &i2c_flash_api;
	}

	return ret;
}

static struct i2c_flash_data i2c_flash_memory_data;

DEVICE_INIT(i2c_flash_memory, CONFIG_I2C_FLASH_W25QXXDV_DRV_NAME, i2c_flash_init,
	    &i2c_flash_memory_data, NULL, POST_KERNEL,
	    CONFIG_I2C_FLASH_W25QXXDV_INIT_PRIORITY);
