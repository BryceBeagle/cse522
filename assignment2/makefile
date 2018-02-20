CC = gcc
CFLAGS = -Wall -O2
CVERSION = -std=c11
LFLAGS = -lm
SOURCES = $(shell find . -name '*.c')
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE=schedule_feasibility

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXECUTABLE) $(LFLAGS) $(CVERSION) $(CFLAGS)

clean:
	rm $(EXECUTABLE)
	find . -name '*.o' -delete
