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
	struct k_sem sem;
};

struct k_sem eeprom_write_sem;


#endif /* __i2c_FLASH_W25QXXDV_H__ */
