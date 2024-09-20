CFLAGS=-Wall -O0 -Wextra -std=c11 -pedantic -g
LIBS=

VirtualMachine: main.c
	gcc $(CFLAGS) -o virtmach main.c $(LIBS)
