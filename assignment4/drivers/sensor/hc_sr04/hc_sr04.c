#include <device.h>
#include <gpio.h>
#include <kernel.h>
#include <sensor.h>
#include <misc/util.h>
#include <misc/__assert.h>
#include <board.h>
#include <pinmux.h>
#include <stdio.h>
#include <sys_clock.h>

#include "hc_sr04.h"

enum pin_level {
	PIN_LOW   = 0x00,
	PIN_HIGH  = 0x01,
	DONT_CARE = 0xFF,
};

#define tsc_read() (_tsc_read())

static void hc_sr04_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins) {

	// Unused arguments
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	// End of measured duration, callback triggered when sensor responds
	u64_t now = tsc_read();

	// Get the drv_data struct
	struct hc_sr04_data *drv_data =
		CONTAINER_OF(cb, struct hc_sr04_data, gpio_cb);

	// Calculate duration from clock cycles
	drv_data->timestamp = now - drv_data->timestamp;
	drv_data->timestamp = SYS_CLOCK_HW_CYCLES_TO_NS64(drv_data->timestamp);

	// Calculate distance from duration
	drv_data->distance = (drv_data->timestamp * 340) / (2 * 10000000);

	// Release semaphore so main loop can continue
	k_sem_give(&drv_data->data_sem);
}

static int hc_sr04_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct hc_sr04_data *drv_data = dev->driver_data;

	k_sem_take(&drv_data->data_sem, K_FOREVER);

	// Start of measured duration
	drv_data->timestamp = tsc_read();

	// Set Trigger pin on sensor high for 10 ms
	gpio_pin_write(drv_data->gpio, drv_data->trig_gpio_pin, PIN_HIGH);
	k_busy_wait(10);
	gpio_pin_write(drv_data->gpio, drv_data->trig_gpio_pin, PIN_LOW);

	// Wait for callback to finish
	if(k_sem_take(&drv_data->data_sem, 30) == -EAGAIN){
		// TODO: Comment why?
		drv_data->timestamp = 400;
	}

	// Release semaphore
	k_sem_give(&drv_data->data_sem);

	return 0;
}


static int hc_sr04_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val) {

	// Using the SENSOR_CHAN_PROX channel because a distance channel does not
	// exist in Zehpyr 1.10.0
	// Also using the two values two store timestamp and distance as ints
	// instead of one float-like value

	struct hc_sr04_data *drv_data = dev->driver_data;

	if (chan == SENSOR_CHAN_PROX) {
		val->val1 = (s32_t) drv_data->timestamp;
		val->val1 = (s32_t) drv_data->distance;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api hc_sr04_driver_api = {
	.sample_fetch = hc_sr04_sample_fetch,
	.channel_get = hc_sr04_channel_get,
};

static int hc_sr04_0_init(struct device *dev)
{
	struct hc_sr04_data *drv_data = dev->driver_data;
	struct device *pinmux = device_get_binding(CONFIG_PINMUX_NAME);

	drv_data->gpio = device_get_binding(CONFIG_HC_SR04_GPIO_DEV_NAME);
	drv_data->trig_gpio_pin = (uint32_t) CONFIG_HC_SR04_0_GPIO_PIN_TRIG;

	if (drv_data->gpio == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device",
			    CONFIG_HC_SR04_GPIO_DEV_NAME);
		return -EINVAL;
	}

	k_sem_init(&drv_data->data_sem, 1, 1);

	/* setup data ready gpio trigger on shield pin 2*/
	pinmux_pin_set(pinmux, CONFIG_HC_SR04_0_SHIELD_PIN_TRIG, PINMUX_FUNC_A);

	// /* setup data ready gpio interrupt on shield pin 0*/
	pinmux_pin_set(pinmux, CONFIG_HC_SR04_0_SHIELD_PIN_ECHO, PINMUX_FUNC_B);
	gpio_pin_configure(drv_data->gpio,  CONFIG_HC_SR04_0_GPIO_PIN_ECHO,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH);

	gpio_init_callback(&drv_data->gpio_cb, hc_sr04_gpio_callback,
	                   BIT(CONFIG_HC_SR04_0_GPIO_PIN_ECHO));
	gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb);
	gpio_pin_enable_callback(drv_data->gpio, CONFIG_HC_SR04_0_GPIO_PIN_ECHO);

	dev->driver_api = &hc_sr04_driver_api;

	return 0;
}

static int hc_sr04_1_init(struct device *dev)
{
	struct hc_sr04_data *drv_data = dev->driver_data;
	struct device *pinmux = device_get_binding(CONFIG_PINMUX_NAME);

	drv_data->gpio = device_get_binding(CONFIG_HC_SR04_GPIO_DEV_NAME);
	drv_data->trig_gpio_pin = (uint32_t) CONFIG_HC_SR04_1_GPIO_PIN_TRIG;

	if (drv_data->gpio == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device",
			    CONFIG_HC_SR04_GPIO_DEV_NAME);
		return -EINVAL;
	}

	k_sem_init(&drv_data->data_sem, 1, 1);

	/* setup data ready gpio trigger on shield pin 3*/
	pinmux_pin_set(pinmux, CONFIG_HC_SR04_1_SHIELD_PIN_TRIG, PINMUX_FUNC_A);

	// /* setup data ready gpio interrupt on shield pin 1*/
	pinmux_pin_set(pinmux, CONFIG_HC_SR04_1_SHIELD_PIN_ECHO, PINMUX_FUNC_B);
	gpio_pin_configure(drv_data->gpio,  CONFIG_HC_SR04_1_GPIO_PIN_ECHO,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH);

	gpio_init_callback(&drv_data->gpio_cb, hc_sr04_gpio_callback,
	                   BIT(CONFIG_HC_SR04_1_GPIO_PIN_ECHO));
	gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb);
	gpio_pin_enable_callback(drv_data->gpio, CONFIG_HC_SR04_1_GPIO_PIN_ECHO);

	dev->driver_api = &hc_sr04_driver_api;

	return 0;
}

static struct hc_sr04_data hc_sr04_0_data;
static struct hc_sr04_data hc_sr04_1_data;

DEVICE_INIT(hc_sr04_0, CONFIG_HC_SR04_0_NAME, hc_sr04_0_init, &hc_sr04_0_data,
	    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);

DEVICE_INIT(hc_sr04_1, CONFIG_HC_SR04_1_NAME, hc_sr04_1_init, &hc_sr04_1_data,
	    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);
