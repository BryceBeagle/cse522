Building the assignment

	Follow the build instructions 1-3 found in section "Building a Sample Application" in http://docs.zephyrproject.org/1.10.0/getting_started/getting_started.html

	Next, move to "ASSIGNMENT_DIR/build/galileo/"
	$ cmake -DBOARD=galileo ../..
	$ make

	Now setup the SD card as per http://docs.zephyrproject.org/1.10.0/boards/x86/galileo/doc/galileo.html and boot.

	The tests will run with outputs indicating the processes and tests being done.

	The tests are over when the line
		"Enter 1, 2, 3 to output data from respective tests"
	is displayed, from which you will be able to see the results of the tests on the 3 previous lines.
		"Test 1  : Interrupt with no background -> XXXX"
		"Test 2  : Interrupt with background -> XXXX"
		"Test 3  : context switch with no background -> XXXX"
	These values are in clock cycles.

	You can also follow the prompt and receive the individual data points used to find the averages displayed prior.


Board Wiring

	The board requires shield pin 3 to be wired to shield pin 0 for the ISR, and optionally shield pin 5 and 9 can be connected to an LED for verification that GPIO is active.