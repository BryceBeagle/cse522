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
#include <pinmux.h>

//view spi_flash_w25qxxdv.c for reference
//must implement checks and retrys for when device is busy

#define ADDRESS_COUNT (32*1024)
#define PAGE_SIZE (64)
#define PAGE_COUNT (ADDRESS_COUNT/PAGE_SIZE)
#define EEPROM_ADDRESS 0xA0
#define READ 0x01

static int i2c_flash_wb_read(struct device *dev, off_t offset, void *data,
			     size_t len) {

	// random sequential read
	// offset is relative to pages rather than bytes
	struct i2c_flash_data *const driver_data = dev->driver_data;
	u8_t read_control_buffer[2];
	int ret = 0;
	k_sem_take(&driver_data->sem, K_FOREVER);

	read_control_buffer[0] = (u8_t) ((64 * offset) & 0xFF00) >> 8;
	read_control_buffer[1] = (u8_t) ((64 * offset) & 0x00FF) >> 0;

	ret |= i2c_write(driver_data->i2c, read_control_buffer, 2, EEPROM_ADDRESS);
	ret |= i2c_read(driver_data->i2c, data, len, EEPROM_ADDRESS | READ);

	k_sem_give(&driver_data->sem);

	printk("Done reading from EEPROM | Ret = %i\n", ret);
	return ret;
}

static int i2c_flash_wb_write(struct device *dev, off_t offset,
			      const void *data, size_t len)
{
	printk("Writing to EEPROM\n");
	// only allow page writes (64 bytes)
	// offset is relative to pages rather than bytes
	struct i2c_flash_data *const driver_data = dev->driver_data;
	u8_t buffer[66];
	int ret = 0;
	k_sem_take(&driver_data->sem, K_FOREVER);

	if(len > 64) {
		ret = -1;
	} else {
		buffer[0] = (u8_t) ((64 * offset) & 0xFF00) >> 8;
		buffer[1] = (u8_t) ((64 * offset) & 0x00FF) >> 0;
		memcpy(buffer+2, data, len);

		ret = i2c_write(driver_data->i2c, buffer, len, EEPROM_ADDRESS);
	}

	k_sem_give(&driver_data->sem);

	printk("Done writing to EEPROM | Ret = %i\n", ret);

	return ret;
}

static int i2c_flash_wb_write_protection_set(struct device *dev, bool enable)
{
	// toggles WP pin to 'enable'
	// no need to implement
	struct i2c_flash_data *const driver_data = dev->driver_data;
	k_sem_take(&driver_data->sem, K_FOREVER);
	k_sem_give(&driver_data->sem);
	return 0;
}

static int i2c_flash_wb_erase(struct device *dev, off_t offset, size_t size)
{
	// call write with an array of zeros for every page
	struct i2c_flash_data *const driver_data = dev->driver_data;
	k_sem_take(&driver_data->sem, K_FOREVER);

	u8_t zeros[64];
	memset(zeros, 0, sizeof(zeros));

	struct device *i2c_dev = device_get_binding(CONFIG_I2C_0_NAME);
	for(off_t i = offset; i < size; i++)
	{
		i2c_flash_wb_write(i2c_dev, i, zeros, sizeof(zeros));
	}

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
	struct i2c_flash_data *const data = dev->driver_data;
	int ret;

	// Faults at program address 0x0008
//	printk("Setting pinmux\n");
//	struct device *pinmux = device_get_binding(CONFIG_PINMUX_NAME);
//	pinmux_pin_set(pinmux, 18, PINMUX_FUNC_C);  // SDA
//	pinmux_pin_set(pinmux, 19, PINMUX_FUNC_C);  // SCL
//	printk("Set pinmux\n");

	i2c_dev = device_get_binding(CONFIG_I2C_0_NAME);
	if (!i2c_dev) {
		return -EIO;
	}

	data->i2c = i2c_dev;
	k_sem_init(&data->sem, 1, UINT_MAX);

	u32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_MASTER;
	ret = i2c_configure(i2c_dev, i2c_cfg);
	if (!ret) {
		dev->driver_api = &i2c_flash_api;
	}

	printk("Ret: %i", ret);

	return ret;
}

static struct i2c_flash_data i2c_flash_memory_data;

DEVICE_INIT(i2c_flash_memory, CONFIG_I2C_FLASH_24FC256_DRV_NAME, i2c_flash_init,
	    &i2c_flash_memory_data, NULL, POST_KERNEL,
	    CONFIG_I2C_FLASH_24FC256_INIT_PRIORITY);
