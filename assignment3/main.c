/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <asm_inline_gcc.h>
#include <gpio.h>
#include <pwm.h>
#include <pinmux.h>
#include <pinmux_galileo.h>
#include <board.h>
#include <misc/printk.h>

#define tsc_read() (_tsc_read())
#define BUFFER_SIZE 500
#define STACKSIZE 1024

enum pin_level {
	PIN_LOW = 0x00,
	PIN_HIGH = 0x01,
	DONT_CARE = 0xFF,
};

typedef struct buffer_api {
	u64_t *buffer;
	unsigned int buffer_index;
	struct k_mutex *buffer_lock;
} buffer_api;

u64_t interrupt_no_background_buffer[BUFFER_SIZE];
u64_t interrupt_w_background_buffer[BUFFER_SIZE];
u64_t context_switch_buffer[BUFFER_SIZE];
buffer_api current_buffer;
K_THREAD_STACK_DEFINE(stack_area_0, STACKSIZE);
K_THREAD_STACK_DEFINE(stack_area_1, STACKSIZE);

//test 1 + 2 variables
struct k_mutex *ready_state_mutex;
int ready_state;
void ready_state_take(int *value);
void ready_state_set(int value);
void sig_got(struct device *gpiob, struct gpio_callback *cb, u32_t pins);

//test 2 variables
K_SEM_DEFINE(threadA_sem, 1, 1);	/* starts off "available" */
K_SEM_DEFINE(threadB_sem, 0, 1);	/* starts off "not available" */
static struct gpio_callback gpio_cb;
static struct k_thread threadA_data;
static struct k_thread threadB_data;
void passer_thread(void *my_sem, void *other_sem, void *dummy3);

//test 3 variables
K_MUTEX_DEFINE(context_switch_mutex);
static struct k_thread context_switch_higher_thread;
static struct k_thread context_switch_lower_thread;
void context_switch_higher(void *my_sem, void *other_sem, void *dummy3);
void context_switch_lower(void *my_sem, void *other_sem, void *dummy3);

void ready_state_take(int *value)
{
	k_mutex_lock(ready_state_mutex, K_FOREVER);
	*value = ready_state;
	ready_state = 0;
	k_mutex_unlock(ready_state_mutex);
}

void ready_state_set(int value)
{
	k_mutex_lock(ready_state_mutex, K_FOREVER);
	ready_state = value;
	k_mutex_unlock(ready_state_mutex);
}

void passer_thread(void *my_sem, void *other_sem, void *dummy3)
{
	ARG_UNUSED(dummy3);
	// printk("%llu Starting", k_current_get());

	/* invoke routine to ping-pong hello messages with threadA */
	while (1) {
		/* take my semaphore */
		k_yield();
		k_sem_take(my_sem, K_FOREVER);
		// printk("My Turn %llu", k_current_get());
		/* wait a while, then let other thread have a turn */
		k_sem_give(other_sem);
	}
}

void context_switch_higher(void *dummy1, void *dummy2, void *dummy3)
{
	while(current_buffer.buffer_index < BUFFER_SIZE)
	{
		k_sleep(10);
		ready_state_set(1);

		k_mutex_lock(&context_switch_mutex, K_FOREVER);
		*(current_buffer.buffer) = tsc_read() - *(current_buffer.buffer);
		current_buffer.buffer++;
		current_buffer.buffer_index++;
		k_mutex_unlock(&context_switch_mutex);
	}
}

void context_switch_lower(void *dummy1, void *dummy2, void *dummy3)
{
	while(current_buffer.buffer_index < BUFFER_SIZE)
	{
		k_mutex_lock(&context_switch_mutex, K_FOREVER);
		int ready;
		do{
			ready_state_take(&ready);
		}while(ready == 0);
		*(current_buffer.buffer) = tsc_read();
		k_mutex_unlock(&context_switch_mutex);
	}
}

void sig_got(struct device *gpiob, struct gpio_callback *cb, u32_t pins)
{
	*(current_buffer.buffer) = tsc_read() - *(current_buffer.buffer);
	printk("SIG GOT\n");
	current_buffer.buffer_index++;
	current_buffer.buffer++;
	ready_state_set(1);
}

void setup()
{
	k_mutex_init(ready_state_mutex);
	ready_state_set(0);

	struct device *EXP1 = device_get_binding(PINMUX_GALILEO_EXP1_NAME);
	struct device *GPIO_0 = device_get_binding(PINMUX_GALILEO_GPIO_DW_NAME);
	struct device *PWM0 = device_get_binding(PINMUX_GALILEO_PWM0_NAME);

	//SHIELD PIN 3
	if(pinmux_pin_set(device_get_binding(CONFIG_PINMUX_NAME), 3, PINMUX_FUNC_A)){
		printk("ERROR 0.1\n");
	}
	//SHIELD PIN 5
	if(pinmux_pin_set(device_get_binding(CONFIG_PINMUX_NAME), 5, PINMUX_FUNC_A)){
		printk("ERROR 0.2\n");
	}
	//SHIELD PIN 9
	if(pinmux_pin_set(device_get_binding(CONFIG_PINMUX_NAME), 9, PINMUX_FUNC_C)){
		printk("ERROR 0.3\n");
	}

	//SHIELD PIN 0
	// { EXP1,  0,   PIN_HIGH, (GPIO_DIR_OUT) }, /* GPIO3 int */
	// { EXP1,  1,   PIN_LOW, (GPIO_DIR_OUT) },
	// { G_DW,  3,   PIN_LOW, (GPIO_DIR_IN)  },
	if(gpio_pin_configure(EXP1,    0, GPIO_DIR_OUT) ||
	   gpio_pin_configure(EXP1,    1, GPIO_DIR_OUT) ||
	   gpio_pin_configure(GPIO_0,  3, GPIO_DIR_IN | GPIO_INT | GPIO_INT_ACTIVE_HIGH | GPIO_INT_EDGE) )
	{
		printk("ERROR 0.4\n");
		return;
	}

	gpio_init_callback(&gpio_cb, sig_got, BIT(3));
	gpio_add_callback(GPIO_0, &gpio_cb);
	gpio_pin_enable_callback(GPIO_0, 3);

	if(gpio_pin_write(EXP1,    0, PIN_HIGH) ||
	   gpio_pin_write(EXP1,    1, PIN_LOW)  )
	{
		printk("ERROR 0.5\n");
		return;
	}


}

void main(void)
{
	printk("Testing Response Time!\n");
	setup();
	struct device *GPIO_0 = device_get_binding(PINMUX_GALILEO_GPIO_DW_NAME);
	struct device *G_CW = device_get_binding(PINMUX_GALILEO_GPIO_INTEL_CW_NAME);
	struct device *PWM0 = device_get_binding(PINMUX_GALILEO_PWM0_NAME);

	printk("Running\n");

	/*
	 * I'm having trouble setting up PWM at the moment, everything else is done.
	 */
	// current_buffer.buffer_index = 0;
	// current_buffer.buffer = interrupt_w_background_buffer;

	// {
		// printk("Setting-1 : %d\n", pwm_pin_set_cycles(PWM0,  7, 100, 4096));
	// 	k_sleep(2000);
		// printk("Setting0 : %d\n", pwm_pin_set_cycles(PWM0,  7, 100, 1000));
		// k_sleep(2000);
		// printk("Setting1 : %d\n", pwm_pin_set_cycles(PWM0,  7, 100, 1500));
		// k_sleep(2000);
		// printk("Setting2 : %d\n", pwm_pin_set_cycles(PWM0,  7, 100, 2000));
		// k_sleep(2000);
		// printk("Setting3 : %d\n", pwm_pin_set_cycles(PWM0,  7, 100, 2500));
		// k_sleep(2000);
		// printk("Setting4 : %d\n", pwm_pin_set_cycles(PWM0,  7, 100, 3000));
		// k_sleep(2000);
		// printk("Setting5 : %d\n", pwm_pin_set_cycles(PWM0,  7, 100, 3500));
		// k_sleep(2000);
		// printk("Setting6 : %d\n", pwm_pin_set_cycles(PWM0,  7, 100, 4000));
		// k_sleep(2000);
		printk("Setting7 : %d\n", pwm_pin_set_cycles(PWM0,  7, 100, 4500));
		k_sleep(2000);
		// printk("Setting8 : %d\n", pwm_pin_set_cycles(PWM0,  7, 100, 5000));
		// k_sleep(2000);
		// printk("Setting4 : %d\n", pwm_pin_set_cycles(PWM0,  7, 100, 3000));
		// k_sleep(2000);
	// }

	// no background
	printk("Running Test 1\n");
	{
		current_buffer.buffer_index = 0;
		current_buffer.buffer = interrupt_no_background_buffer;
		while(current_buffer.buffer_index < BUFFER_SIZE)
		{

			// k_sleep(50);
			*(current_buffer.buffer) = tsc_read();
			gpio_pin_write(GPIO_0, 6, 1);
			gpio_pin_write(GPIO_0, 6, 0);
			gpio_pin_write(G_CW,   0, 1);
			gpio_pin_write(G_CW, 0, 0);

			int ready;
			do{
				ready_state_take(&ready);
			}while(ready == 0);

			// k_sleep(50);

		}
	}

	//with background
	printk("Running Test 2\n");
	{
		current_buffer.buffer_index = 0;
		current_buffer.buffer = interrupt_w_background_buffer;
		k_tid_t ta = k_thread_create(&threadA_data, stack_area_0, STACKSIZE,
				passer_thread, &threadA_sem, &threadB_sem, NULL,
				k_thread_priority_get(k_current_get()), 0, K_NO_WAIT);
		k_tid_t tb = k_thread_create(&threadB_data, stack_area_1, STACKSIZE,
				passer_thread, &threadB_sem, &threadA_sem, NULL,
				k_thread_priority_get(k_current_get()), 0, K_NO_WAIT);
		while(current_buffer.buffer_index < BUFFER_SIZE)
		{
			k_sleep(50);

			*(current_buffer.buffer) = tsc_read();


			// gpio_pin_write(GPIO_0, 6, 1);
			// gpio_pin_write(G_CW,   0, 1);

			int ready;
			do{
				ready_state_take(&ready);
				k_sleep(1020);
				printk("retry\n");
			}while(ready == 0);

			k_sleep(50);

			// gpio_pin_write(GPIO_0, 6, 0);
			// gpio_pin_write(G_CW,   0, 0);
		}
		k_thread_abort(ta);
		k_thread_abort(tb);
	}

	//context switch
	printk("Running Test 3\n");
	{
		ready_state_set(0);
		current_buffer.buffer_index = 0;
		current_buffer.buffer = context_switch_buffer;
		k_tid_t t1 = k_thread_create(&context_switch_higher_thread, stack_area_0, STACKSIZE,
				context_switch_higher, NULL, NULL, NULL, 5, 0, 200);
		k_tid_t t2 = k_thread_create(&context_switch_lower_thread, stack_area_1, STACKSIZE,
				context_switch_lower, NULL, NULL, NULL, 10, 0, 200);
		while(current_buffer.buffer_index < BUFFER_SIZE)
		{
			k_sleep(500);
		}
		k_thread_abort(t1);
		k_thread_abort(t2);
	}

	//results
	{
		int average_tcl_int_no_background = 0;
		int average_tcl_int_w_background = 0;
		int average_tcl_int_context_switch = 0;
		current_buffer.buffer = interrupt_no_background_buffer;
		for (int i = 0; i < BUFFER_SIZE; i++)
		{
			average_tcl_int_no_background += current_buffer.buffer[i];
		}
		average_tcl_int_no_background /= 500;

		current_buffer.buffer = interrupt_w_background_buffer;
		for (int i = 0; i < BUFFER_SIZE; i++)
		{
			average_tcl_int_w_background += current_buffer.buffer[i];
		}
		average_tcl_int_w_background /= 500;

		current_buffer.buffer = context_switch_buffer;
		for (int i = 0; i < BUFFER_SIZE; i++)
		{
			average_tcl_int_context_switch += current_buffer.buffer[i];
		}
		average_tcl_int_context_switch /= 500;

		printk("Test 1 : Interrupt with no background -> %d\n", average_tcl_int_no_background);
		printk("Test 2 : Interrupt with background -> %d\n", average_tcl_int_w_background);
		printk("Test 3 : context switch with no background -> %d\n", average_tcl_int_context_switch);
	}
}
