Assignment 1 readme

Compiling the program:
	To build for local, or desktop environments, run
		make local
	To build for the Galileo, run
		make galileo

	To build either for local or for the galileo with PI-Mutex, include 'PI=on'. I.E.
		make galileo PI=on

	A executable, named main.exe, will be created for the desired platform.

Running the program
	The program needs root access for mouse reading and priority setting.
	The program can accept either an input file as it's first or can be fed the input through stdin, if desired.

	stdin example
		sudo ./main.exe < input_file
	file example
		sudo ./main.exe input_file

	Thread ID's and priority settings will be displayed to the console, followed by readouts of the operations as the are completed.

Cleaning the program:
	To delete the compiled executable, simply run
		make clean
