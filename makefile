$(CC) = gcc

all: executable

debug: CC += -DDEBUG -g
debug: executable

executable:
	$(CC) filewatcher.c textcolor.c -o filewatcher -pthread
