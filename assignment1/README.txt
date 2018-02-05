Assignment 1 readme

Compiling the program:
	To build for local, or desktop environments, run
		make local
	To build for the Galileo, run
		make galileo

	To build either for local or for the galileo with PI-Mutex, include 'PI=on'. I.E.
		make galileo PI=on

	A executable, named main.exe, will be created for the desired platform.

Cleaning the program:
	To delete the compiled executable, simply run
		make clean
