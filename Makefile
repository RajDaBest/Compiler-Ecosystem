CFLAGS=-Wall -O0 -Wextra -std=c11 -pedantic -g
LIBS=

VirtualMachine: src/main.c
	gcc $(CFLAGS) -o virtmach src/main.c $(LIBS)
