/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <console.h>
#include <gpio.h>
#include <pwm.h>
#include <pinmux.h>
#include <board.h>

#define tsc_read() (_tsc_read())
#define BUFFER_SIZE 500
#define STACKSIZE 1024

enum pin_level {
	PIN_LOW = 0x00,
	PIN_HIGH = 0x01,
	DONT_CARE = 0xFF,
};

typedef struct buffer_api {
	u64_t * volatile buffer;
	unsigned int volatile buffer_index;
} buffer_api;

u64_t interrupt_no_background_buffer[BUFFER_SIZE];
u64_t interrupt_w_background_buffer_reference[BUFFER_SIZE];
u64_t interrupt_w_background_buffer[BUFFER_SIZE];
u64_t context_switch_buffer[BUFFER_SIZE];
buffer_api volatile current_buffer;
K_THREAD_STACK_DEFINE(stack_area_0, STACKSIZE);
K_THREAD_STACK_DEFINE(stack_area_1, STACKSIZE);

//test 1 + 2 variables
struct k_mutex *ready_state_mutex;
static struct gpio_callback gpio_cb;
K_SEM_DEFINE(int_sem, 0, 501);		/* starts off "not available" */
K_SEM_DEFINE(done_mut, 0, 1);		/* starts off "not available" */
void sig_got(struct device *gpiob, struct gpio_callback *cb, u32_t pins);

//test 2 variables
K_SEM_DEFINE(threadA_sem, 1, 1);	/* starts off "available" */
K_SEM_DEFINE(threadB_sem, 0, 1);	/* starts off "not available" */
static struct k_thread threadA_data;
static struct k_thread threadB_data;
void passer_thread(void *my_sem, void *other_sem, void *dummy3);

//test 3 variables
K_MUTEX_DEFINE(context_switch_mutex);
K_MUTEX_DEFINE(interupt_mutex);
static struct k_thread context_switch_higher_thread;
static struct k_thread context_switch_lower_thread;
void context_switch_higher(void *my_sem, void *other_sem, void *dummy3);
void context_switch_lower(void *my_sem, void *other_sem, void *dummy3);

void passer_thread(void *my_sem, void *other_sem, void *dummy3)
{
	ARG_UNUSED(dummy3);

	/* invoke routine to ping-pong hello messages with threadA */
	while (1) {
		/* take my semaphore */
		k_sem_take(my_sem, K_FOREVER);
		/* wait a while */
		printk("Starting\n");
		k_busy_wait(500);
		/* let other thread have a turn */
		k_sem_give(other_sem);
		k_yield();
	}
}

void context_switch_higher(void *dummy1, void *dummy2, void *dummy3)
{
	while(current_buffer.buffer_index < BUFFER_SIZE)
	{
		k_sleep(10); // yield for the lower priority thread to lock

		k_mutex_lock(&context_switch_mutex, K_FOREVER); // wait for lower thread to fill current_buffer.buffer
		*(current_buffer.buffer) = tsc_read() - *(current_buffer.buffer); // find difference from lower to higher time
		current_buffer.buffer++;
		current_buffer.buffer_index++;
		k_mutex_unlock(&context_switch_mutex); // signify that the higher task finished calculation
	}
}

void context_switch_lower(void *dummy1, void *dummy2, void *dummy3)
{
	while(current_buffer.buffer_index < BUFFER_SIZE)
	{
		k_mutex_lock(&context_switch_mutex, K_FOREVER); // stop higher task from running
		*(current_buffer.buffer) = tsc_read(); // fill current_buffer.buffer
		k_mutex_unlock(&context_switch_mutex); // allow higher task to run
	}
}

void sig_got(struct device *gpiob, struct gpio_callback *cb, u32_t pins)
{
	u64_t now = tsc_read();
	switch(k_sem_count_get(&int_sem)){
	case BUFFER_SIZE:
		k_sem_give(&int_sem);
		k_sem_give(&done_mut);
		break;
	case BUFFER_SIZE+1:
		printk("Ending\n");
		gpio_pin_disable_callback(device_get_binding(PINMUX_GALILEO_GPIO_DW_NAME), 3);
		break;
	default:
		printk("Magic Print %d\n", k_sem_count_get(&int_sem));
		current_buffer.buffer[current_buffer.buffer_index] = now - current_buffer.buffer[current_buffer.buffer_index];
		current_buffer.buffer_index++;
		k_sem_give(&int_sem);
		break;
	}
}

void setup()
{

	struct device *EXP1   = device_get_binding(PINMUX_GALILEO_EXP1_NAME);
	struct device *GPIO_0 = device_get_binding(PINMUX_GALILEO_GPIO_DW_NAME);
	struct device *PWM0   = device_get_binding(PINMUX_GALILEO_PWM0_NAME);

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

	// Turn on PWM Counter
	pwm_pin_set_cycles(PWM0, 1, 0, 0);
	pwm_pin_set_cycles(PWM0, 7, 0, 0);
	pwm_pin_set_cycles(PWM0, 7, 0, 100);
	pwm_pin_set_cycles(PWM0, 1, 0, 100);

	//SHIELD PIN 0
	// { EXP1,  0,   PIN_HIGH, (GPIO_DIR_OUT) }, /* GPIO3 int */
	// { EXP1,  1,   PIN_LOW, (GPIO_DIR_OUT) },
	// { G_DW,  3,   PIN_LOW, (GPIO_DIR_IN)  },
	if(gpio_pin_configure(EXP1,    0, GPIO_DIR_OUT) ||
	   gpio_pin_configure(EXP1,    1, GPIO_DIR_OUT) ||
	   gpio_pin_configure(GPIO_0,  3, GPIO_DIR_IN | GPIO_INT | GPIO_INT_ACTIVE_HIGH | GPIO_INT_EDGE) ) //rising edge trigger interrupt
	{
		printk("ERROR 0.4\n");
		return;
	}

	gpio_init_callback(&gpio_cb, sig_got, BIT(3));
	gpio_add_callback(GPIO_0, &gpio_cb);

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
	struct device *GPIO_0 = device_get_binding(PINMUX_GALILEO_GPIO_DW_NAME);
	struct device *G_CW = device_get_binding(PINMUX_GALILEO_GPIO_INTEL_CW_NAME);

	setup();

	// no background
	// First test using with no background distortion, using GPIO for accurate time analysis
	printk("Running Test 1\n");
	{
		//setup buffer pointer
		current_buffer.buffer_index = 0;
		current_buffer.buffer = interrupt_no_background_buffer;
		k_sem_reset(&int_sem);  // release the semaphore
		gpio_pin_enable_callback(GPIO_0, 3);  // activate the ISR
		while(k_sem_take(&done_mut, K_NO_WAIT) != 0) // exit when the ISR fill the buffer and releases the sem
		{
			current_buffer.buffer[current_buffer.buffer_index] = tsc_read(); // get gpio on time, the ISR will then find the difference from its time
			gpio_pin_write(GPIO_0, 6, 1);
			gpio_pin_write(G_CW,   0, 1);
			k_sleep(5);

			gpio_pin_write(GPIO_0, 6, 0);
			gpio_pin_write(G_CW,   0, 0);
			k_sleep(5);
		}
	}

	//with background
	// Second test with background distortion
	// Using PWM for asynchronous signals to analyze the effect threads switching has on interrupt latency
	printk("Running Test 2\n");
	{
		// Configure the shield pin to PWM
		pinmux_pin_set(device_get_binding(CONFIG_PINMUX_NAME), 3, PINMUX_FUNC_C);

		for (int i = 0; i < BUFFER_SIZE; i++)
		{
			interrupt_w_background_buffer_reference[i] = 0;
			interrupt_w_background_buffer[i] = 0;
		}

		// get with background data

		// activate threads
		k_tid_t ta = k_thread_create(&threadA_data, stack_area_0, STACKSIZE,
				passer_thread, &threadA_sem, &threadB_sem, NULL,
				k_thread_priority_get(k_current_get())+1, 0, K_NO_WAIT);
		k_tid_t tb = k_thread_create(&threadB_data, stack_area_1, STACKSIZE,
				passer_thread, &threadB_sem, &threadA_sem, NULL,
				k_thread_priority_get(k_current_get())+1, 0, K_NO_WAIT);

		// see test 1 for explanation to similar code block
		current_buffer.buffer_index = 0;
		current_buffer.buffer = interrupt_w_background_buffer;
		k_sem_reset(&int_sem);
		gpio_pin_enable_callback(GPIO_0, 3);
		while(k_sem_take(&done_mut, K_NO_WAIT) != 0)
		{
			k_sleep(500); //allow the lower priority task to run
		}

		k_thread_abort(ta); // kill the useless task to keep the scheduler clean
		k_thread_abort(tb); // kill the useless task to keep the scheduler clean

		k_sleep(500);
		printk("Aborted Threads\n");

		//get reference data
		current_buffer.buffer_index = 0;
		current_buffer.buffer = interrupt_w_background_buffer_reference;
		k_sem_reset(&int_sem);
		gpio_pin_enable_callback(GPIO_0, 3);
		k_sem_take(&done_mut, K_FOREVER); // No need to sleep since no other threads are running
	}

	//context switch
	printk("Running Test 3\n");
	{
		current_buffer.buffer_index = 0;
		current_buffer.buffer = context_switch_buffer;
		k_tid_t t1 = k_thread_create(&context_switch_higher_thread, stack_area_0, STACKSIZE,
				context_switch_higher, NULL, NULL, NULL, k_thread_priority_get(k_current_get())+1, 0, 200); // create threads of slightly lower priority to main
		k_tid_t t2 = k_thread_create(&context_switch_lower_thread, stack_area_1, STACKSIZE,
				context_switch_lower, NULL, NULL, NULL, k_thread_priority_get(k_current_get())+5, 0, 200); // create threads of slightly lower priority to main
		while(current_buffer.buffer_index < BUFFER_SIZE) // wait for threads to fill
		{
			k_sleep(500);
		}
		k_thread_abort(t1);
		k_thread_abort(t2);
	}

	//results
	{
		int average_tcl_int_no_background = 0;
		int average_tcl_int_w_background_reference = 0;
		int average_tcl_int_w_background = 0;
		int average_tcl_int_context_switch = 0;

		//TEST 1///////////////////////////////////
		// get average delay
		current_buffer.buffer = interrupt_no_background_buffer;
		for (int i = 0; i < BUFFER_SIZE; i++)
		{
			average_tcl_int_no_background += current_buffer.buffer[i];
		}
		average_tcl_int_no_background /= 500;

		//TEST 2///////////////////////////////////
		// we have no reference for the PWM start time, so we have to use relative delay to find effect of the scheduling
		{
			int pwm_period_reference = 0;
			int pwm_start_reference = 0;

			// get average period from the data with no threads running
			current_buffer.buffer = interrupt_w_background_buffer_reference;
			for (int i = 1; i < BUFFER_SIZE; i++)
			{
				pwm_period_reference += current_buffer.buffer[i] - current_buffer.buffer[i-1];
			}
			pwm_period_reference /= 499;
			//get an estimated starting time using the reliable GPIO delay
			pwm_start_reference = (current_buffer.buffer[0] % pwm_period_reference) - average_tcl_int_no_background;

			// get average delay of the none background effected data, using the ideal start and period calculated above
			for (int i = 0; i < BUFFER_SIZE; i++)
			{
				average_tcl_int_w_background_reference += ((current_buffer.buffer[i] % pwm_period_reference) - pwm_start_reference) % pwm_period_reference;
			}
			average_tcl_int_w_background_reference /= 500;

			// get average delay of the background effected data, using the ideal start and period calculated above
			current_buffer.buffer = interrupt_w_background_buffer;
			for (int i = 0; i < BUFFER_SIZE; i++)
			{
				average_tcl_int_w_background += ((current_buffer.buffer[i] % pwm_period_reference) - pwm_start_reference) % pwm_period_reference;
			}
			average_tcl_int_w_background /= 500;
			// The effect of the scheduler on the interrupt latency will be seen in the difference between the average delay of the two average times calculated above
		}

		//TEST 3///////////////////////////////////
		// get average delay
		current_buffer.buffer = context_switch_buffer;
		for (int i = 0; i < BUFFER_SIZE; i++)
		{
			average_tcl_int_context_switch += current_buffer.buffer[i];
		}
		average_tcl_int_context_switch /= 500;

		// Start prompt for user to request data set
		printk("Test 1  : Interrupt with no background \t\t-> %d\n", average_tcl_int_no_background);
		printk("Test 2  : Interrupt with background \t\t-> %d\n", (average_tcl_int_w_background > average_tcl_int_w_background_reference ?
			average_tcl_int_w_background - average_tcl_int_w_background_reference :
			average_tcl_int_w_background_reference - average_tcl_int_w_background));
		printk("Test 3  : context switch with no background \t-> %d\n", average_tcl_int_context_switch);
		console_getline_init();
		while(1){
			printk("Enter 1, 2, 3 to output data from respective tests\n");
			char *choice = console_getline();
			switch(choice[0]){
			case '1':
			{
				current_buffer.buffer = interrupt_no_background_buffer;
				for (int i = 0; i < BUFFER_SIZE; i++)
				{
					printk("%llu\n", current_buffer.buffer[i]);
				}
				break;
			}
			case '2':
			{
				int pwm_period_reference = 0;
				int pwm_start_reference = 0;

				current_buffer.buffer = interrupt_w_background_buffer_reference;
				for (int i = 1; i < BUFFER_SIZE; i++)
				{
					pwm_period_reference += current_buffer.buffer[i] - current_buffer.buffer[i-1];
				}
				pwm_period_reference /= 499;
				pwm_start_reference = (current_buffer.buffer[0] % pwm_period_reference) - average_tcl_int_no_background;

				for (int i = 0; i < BUFFER_SIZE; i++)
				{
					int br = ((interrupt_w_background_buffer_reference[i] % pwm_period_reference) - pwm_start_reference) % pwm_period_reference;
					int b  = ((interrupt_w_background_buffer[i] % pwm_period_reference) - pwm_start_reference) % pwm_period_reference;
					printk("%d\n", (b > br ? b - br : br - b));
				}
				break;
			}
			case '3':
			{
				current_buffer.buffer = context_switch_buffer;
				for (int i = 0; i < BUFFER_SIZE; i++)
				{
					printk("%llu\n", current_buffer.buffer[i]);
				}
				break;
			}
			}
		}
	}
}
