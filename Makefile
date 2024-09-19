CFLAGS=-Wall -Wextra -std=c11 -pedantic
LIBS=

VirtualMachine: main.c
	gcc $(CFLAGS) -o virtmach main.c $(LIBS)
