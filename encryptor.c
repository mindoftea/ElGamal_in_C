#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "intChain.h"

static uint64_t keySize;
static struct intChain* PrimeModulus;
static struct intChain* Generator;
static struct intChain* Exponential;

FILE* fp;

static void encryptWord(
    char* word
) {
	struct intChain* IntWord = intEncodeString(word);
	struct intChain* Scramble = intCryptoRandom(PrimeModulus);
	struct intChain* ScrambleCipher = intModExp(Generator, Scramble, PrimeModulus);
	struct intChain* ScrambledExponential = intModExp(Exponential, Scramble, PrimeModulus);
	struct intChain* WordCipher = intMult(IntWord, ScrambledExponential);
	intMod(WordCipher, PrimeModulus);
	char* ScrambleCipherString = intToString(ScrambleCipher);
	char* WordCipherString = intToString(WordCipher);
	fprintf(fp, "%s\n%s\n\n", ScrambleCipherString, WordCipherString);
	free(ScrambleCipherString);
	free(WordCipherString);
	intFree(IntWord);
	intFree(Scramble);
	intFree(ScrambleCipher);
	intFree(ScrambledExponential);
	intFree(WordCipher);
}

static uint32_t readWord(
    char* word
) {
	uint32_t stillReading = 1;
	uint32_t k = 0;
	while (k < keySize / 16) {
		char c = getchar();
		if (c == EOF) {
			stillReading = 0;
			break;
		}
		word[k++] = c;
	}
	encryptWord(word);
	return stillReading;
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Usage: %s publicKeyFile cipherTextFile\n", argv[0]);
		return 1;
	}
	fp = fopen(argv[1], "r");
	if(fp == 0) {
		printf("Couldn't open publicKeyFile.");
		return 2;
	}
	keySize = 0;
	if (fscanf(fp, "%*12[^/]%lu", &keySize) != 1) {
		printf("The public key file is improperly formatted. %lu\n", keySize);
		return 3;
	}
	char* string = malloc((keySize / 64 + 1) * 17 + 1);
	if (fscanf(fp, "%*22[^/]%[^\n]", string) != 1) {
		printf("The public key file is improperly formatted.\n");
		return 4;
	}
	PrimeModulus = intFromString(string);
	if (fscanf(fp, "%*13[^/]%[^\n]", string) != 1) {
		printf("The public key file is improperly formatted.\n");
		return 5;
	}
	Generator = intFromString(string);
	if (fscanf(fp, "%*15[^/]%[^\n]", string) != 1) {
		printf("The public key file is improperly formatted.\n");
		return 6;
	}
	Exponential = intFromString(string);
	fclose(fp);
	fp = fopen(argv[2], "w");
	memset(string, 0, keySize * 17 / 64 + 2);
	while (readWord(string)) {
		memset(string, 0, keySize * 17 / 64 + 2);
	};
	fclose(fp);
	free(string);
	intFree(PrimeModulus);
	intFree(Generator);
	intFree(Exponential);
	return 0;
}
