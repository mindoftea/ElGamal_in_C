#include "intChain.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	struct intChain* Size = intMake();
	intIncrement(Size);
	intLShift(Size, 1024);
	struct intChain* X = intPseudoRandom(Size);
	struct intChain* Y = intPseudoRandom(Size);
	struct intChain* Z = intPseudoRandom(Size);
	struct intChain* W = intModExp(X, Y, Z);
	intFree(X);
	intFree(Y);
	intFree(Z);
	intFree(W);
	return 0;
}
