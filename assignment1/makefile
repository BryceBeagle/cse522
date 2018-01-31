SDK_HOME = /opt/iot-devkit/1.7.2/sysroots

PATH := $(PATH):$(SDK_HOME)/x86_64-pokysdk-linux/usr/bin/i586-poky-linux

CC = i586-poky-linux-gcc
CCLOCAL = gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-
SROOT=$(SDK_HOME)/i586-poky-linux/
CFLAGS=-Wall -lpthread -std=c99


galileo: main.c
	$(CC) main.c -o main.exe --sysroot=$(SROOT) $(CFLAGS)

local: main.c
	$(CCLOCAL) main.c -o main.exe $(CFLAGS)

clean:
	rm -f *.exe
