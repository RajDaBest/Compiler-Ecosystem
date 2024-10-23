CFLAGS=-Wall -Wextra -std=c11 -pedantic -O3 -Wall -march=native -ffast-math -funroll-loops -ftree-vectorize -finline-functions -floop-nest-optimize -mavx2 -mfma -mfpmath=sse -flto -fno-math-errno -fno-signed-zeros -pg -g 
LIBS=

nan_virtmach: src/nan_boxed/main.c
	gcc $(CFLAGS) -o ./bin/virtmach src/nan_boxed/main.c $(LIBS)

nan_devasm: src/nan_boxed/devasm.c 
	gcc $(CFLAGS) -o ./bin/devasm src/nan_boxed/devasm.c $(LIBS)

nan_vpp: src/nan_boxed/vpp.c 

	gcc $(CFLAGS) -o ./bin/vpp src/nan_boxed/vpp.c $(LIBS)

nan_clean:
	rm -f ./bin/nanboxed/nan_virtmach ./bin/nanboxed/nan_devasm ./bin/nanboxed/nan_vpp ./bin/compiler/vtx

clean:

	rm -f ./bin/non_nanboxed/virtmach ./bin/non_nanboxed/devasm ./bin/non_nanboxed/vpp

virtmach: src/non_nanboxed/main.c
		gcc $(CFLAGS) -o ./bin/non_nanboxed/virtmach src/non_nanboxed/main.c $(LIBS)

devasm: src/non_nanboxed/devasm.c
		gcc $(CFLAGS) -o ./bin/non_nanboxed/devasm src/non_nanboxed/devasm.c $(LIBS)
	
vpp: src/non_nanboxed/vpp.c
		gcc $(CFLAGS) -o ./bin/non_nanboxed/vpp src/non_nanboxed/vpp.c $(LIBS)

compiler: src/Compiler-Backend/vasm2amd64.c 
		gcc $(CFLAGS) -o ./bin/compiler/vtx src/Compiler-Backend/vasm2amd64.c $(LIBS)

