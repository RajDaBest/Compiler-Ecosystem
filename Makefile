CFLAGS=-Wall -Wextra -std=c11 -pedantic -O3 -Wall -march=native -ffast-math -funroll-loops -ftree-vectorize -finline-functions -floop-nest-optimize -mavx2 -mfma -mfpmath=sse -flto -fno-math-errno -fno-signed-zeros -pg -g 
LIBS=

virtmach: src/main.c
	gcc $(CFLAGS) -o virtmach src/main.c $(LIBS)

devasm: src/devasm.c 
	gcc $(CFLAGS) -o devasm src/devasm.c $(LIBS)

clean:
	rm -f virtmach devasm
