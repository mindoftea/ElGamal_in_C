#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "intChain.h"

static uint64_t keySize;
static struct intChain* PrimeModulus;
static struct intChain* Generator;
static struct intChain* Exponent;

FILE* fp;

static void decryptWord(
    struct intChain* ScrambleCipher,
    struct intChain* WordCipher
) {
	struct intChain* PrimeModulusMinusTwo = intCopy(PrimeModulus);
	intDecrement(PrimeModulusMinusTwo);
	intDecrement(PrimeModulusMinusTwo);
	struct intChain* Cipher = intModExp(ScrambleCipher, Exponent, PrimeModulus);
	struct intChain* CipherInverse = intModExp(Cipher, PrimeModulusMinusTwo, PrimeModulus);
	// struct intChain* temp = intMult(Cipher, CipherInverse);
	// intMod(temp, PrimeModulus);
	// printf("\n\n--> %s\n\n", intToString(temp));
	struct intChain* EncodedPlaintext = intMult(WordCipher, CipherInverse);
	intMod(EncodedPlaintext, PrimeModulus);
	char* plaintext = intDecodeString(EncodedPlaintext);
	printf("%s", plaintext);
	intFree(PrimeModulusMinusTwo);
	intFree(Cipher);
	intFree(CipherInverse);
	intFree(EncodedPlaintext);
	free(plaintext);
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Usage: %s privateKeyFile cipherTextFile\n", argv[0]);
		return 1;
	}
	fp = fopen(argv[1], "r");
	if (fp == 0) {
		printf("Couldn't open privateKeyFile.");
		return 2;
	}
	keySize = 0;
	if (fscanf(fp, "%*13[^/]%lu", &keySize) != 1) {
		printf("The private key file is improperly formatted. %lu\n", keySize);
		return 3;
	}
	fflush(stdout);
	char* string = malloc((keySize / 64 + 1) * 17 + 2);
	if (fscanf(fp, "%*22[^/]%[^\n]", string) != 1) {
		printf("The private key file is improperly formatted.\n");
		return 4;
	}
	PrimeModulus = intFromString(string);
	if (fscanf(fp, "%*13[^/]%[^\n]", string) != 1) {
		printf("The private key file is improperly formatted.\n");
		return 5;
	}
	Generator = intFromString(string);
	if (fscanf(fp, "%*12[^/]%[^\n]", string) != 1) {
		printf("The private key file is improperly formatted.\n");
		return 6;
	}
	Exponent = intFromString(string);
	fclose(fp);
	fp = fopen(argv[2], "r");
	uint32_t stillReading = 1;
	while (stillReading) {
		if (fscanf(fp, "%[^\n]%*1[^/]", string) == EOF) {
			break;
		}
		struct intChain* ScrambleCipher = intFromString(string);
		if (fscanf(fp, "%[^\n]%*2[^/]", string) == EOF) {
			break;
		}
		struct intChain* WordCipher = intFromString(string);
		decryptWord(ScrambleCipher, WordCipher);
		intFree(ScrambleCipher);
		intFree(WordCipher);
	}
	printf("\n");
	fclose(fp);
	free(string);
	intFree(PrimeModulus);
	intFree(Generator);
	intFree(Exponent);
	return 0;
}
