CFLAGS=-Wall -Wextra -std=c11 -pedantic -O3 -Wall -march=native -ffast-math -funroll-loops -ftree-vectorize -finline-functions -floop-nest-optimize -mavx2 -mfma -mfpmath=sse -flto -fno-math-errno -fno-signed-zeros -pg -g 
LIBS=

virtmach: src/main.c
	gcc $(CFLAGS) -o ./bin/virtmach src/main.c $(LIBS)

devasm: src/devasm.c 
	gcc $(CFLAGS) -o ./bin/devasm src/devasm.c $(LIBS)

vpp: src/vpp.c 

	gcc $(CFLAGS) -o ./bin/vpp src/vpp.c $(LIBS)

clean:
	rm -f ./bin/virtmach ./bin/devasm ./bin/vpp
