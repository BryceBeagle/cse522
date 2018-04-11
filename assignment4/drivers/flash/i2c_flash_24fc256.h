/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief This file defines the private data structures for i2c flash driver
 */

#ifndef __I2C_FLASH_W25QXXDV_H__
#define __I2C_FLASH_W25QXXDV_H__


struct i2c_flash_data {
	struct device *i2c;
	u8_t buf[CONFIG_I2C_FLASH_W25QXXDV_MAX_DATA_LEN +
		    W25QXXDV_LEN_CMD_ADDRESS];
	struct k_sem sem;
};


#endif /* __i2c_FLASH_W25QXXDV_H__ */
