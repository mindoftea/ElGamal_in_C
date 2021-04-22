SHELL = /bin/bash

# Conform to the C99 standard:
CFLAGS =  -std=c99

# Remove assertions:
CFLAGS += -DNDEBUG

# Optimize for the native architecture:
CFLAGS += -march=native

# All of the warnings:
CFLAGS += -pedantic -Wall
CFLAGS += -Wshadow -Wextra

# Debugging:
CFLAGS += -g

# Optimization options
OPTIOPTS = -O3

build: encryptor decryptor keyGenerator

decryptor: intChain.o decryptor.c
	gcc ${CFLAGS} intChain.o decryptor.c -o decryptor

encryptor: intChain.o encryptor.c
	gcc ${CFLAGS} intChain.o encryptor.c -o encryptor

keyGenerator: intChain.o keyGenerator.c
	gcc ${CFLAGS} intChain.o keyGenerator.c -o keyGenerator

intChain.o: intChain.c intChain.h intPerf.c intChain.gcda
	gcc ${CFLAGS} ${OPTIOPTS} -fprofile-use intChain.c -c -o intChain.o

intChain.gcda: intChain.c intChain.h intPerf.c
	gcc ${CFLAGS} ${OPTIOPTS} -fprofile-generate intChain.c -c -o intChain.o
	gcc ${CFLAGS} -fprofile-generate intChain.o intPerf.c -o intPerf
	./intPerf

clean:
	rm -f intChain.o intPerf keyGenerator encryptor decryptor intChain.gcda intPerf.gcda
