#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "intChain.h"

int main(int argc, char* argv[]) {
	if (argc < 4) {
		printf("Usage: %s keySize privateKeyFile publicKeyFile\n", argv[0]);
		return 1;
	}
	char* strEnd;
	// Read the user's choice of key size:
	uint64_t keySize = strtoll(argv[1], &strEnd, 10);
	// Randomly pick a prime modulus to use:
	struct intChain* PrimeModulus = intMakePrime(keySize);
	// Find a random primitive root of the prime modulus to use as a generator:
	struct intChain* Generator = intFindPrimitiveRoot(PrimeModulus);
	// Randomly pick an exponent to encode with:
	struct intChain* Exponent = intCryptoRandom(PrimeModulus);
	// Raise the generator to the chosen exponent, reducing it mod the prime:
	struct intChain* Exponential = intModExp(Generator, Exponent, PrimeModulus);
	// Generate parseable strings of these intChains:
	char* strings[4];
	strings[0] = intToString(PrimeModulus);
	strings[1] = intToString(Generator);
	strings[2] = intToString(Exponent);
	strings[3] = intToString(Exponential);
	FILE *fp = fopen(argv[2], "w");
	if (fp == 0) {
		printf("Failed to open private key output file.");
		return 2;
	}
	// Print out the private key:
	fprintf(
	    fp, "Private Key (%lu bits)\n\nPrimeModulus:\t%s\n\nGenerator:\t%s\n\nExponent:\t%s\n",
	    keySize, strings[0], strings[1], strings[2]
	);
	fclose(fp);
	fp = fopen(argv[3], "w");
	if (fp == 0) {
		printf("Failed to open public key output file.");
		return 3;
	}
	// Print out the public key:
	fprintf(
	    fp, "Public Key (%lu bits)\n\nPrimeModulus:\t%s\n\nGenerator:\t%s\n\nExponential:\t%s\n",
	    keySize, strings[0], strings[1], strings[3]
	);
	fclose(fp);
	free(strings[0]);
	free(strings[1]);
	free(strings[2]);
	free(strings[3]);
	intFree(PrimeModulus);
	intFree(Generator);
	intFree(Exponent);
	intFree(Exponential);
	return 0;
}
