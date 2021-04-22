#pragma once
#include <stdint.h>

// intChain.h
// Emory Hufbauer, 2016

// Structure for heads of intChains:
struct intChain;

// Free the dynamically allocated data in an intChain:
void intFree(
    struct intChain* X  // intChain to be freed
); // O(|X|)

// Make a new intChain with value zero:
struct intChain* intMake(
    void
); // O(1)

// Create a new intChain which is a copy of the given intChain:
struct intChain* intCopy(
    struct intChain* X  // intChain to be copied
); // O(|X|)

// Rightshift an intChain:
//  Wraps intRShiftLarge and intRShiftSmall up together for easy use.
void intRShift(
    struct intChain* X, // intChain to be shifted
    uint64_t n          // Number of bits to shift it by
); // O(|x| + n)

// Leftshift an intChain:
//  Wraps intLShiftLarge and intLShiftSmall up together for easy use.
void intLShift(
    struct intChain* X, // intChain to be shifted
    uint64_t n          // number of bits to shift it by
); // O(|X| + n)

// Given two intChains, X and Y, return a heuristic approximation to log(X/Y):
int64_t __attribute__((pure)) intCompare(
    struct intChain* X, // intChain to be compared
    struct intChain* Y  // intChain to be compared to
); // O(1)

// Given an intChain, X, return a heuristic approximation to log(X):
int64_t __attribute__((pure)) intMagnitude(
    struct intChain* X  // intChain whose size to estimate
); // O(1)

// Given two intChains, X and Y, return 0 if they are equal, 1 if X is greater, and 2 else:
//  Doesn't give false positives, but slow if the arguments are equal or nearly equal.
uint32_t __attribute__((pure)) intFineCompare(
    struct intChain* X, // intChain to be compared
    struct intChain* Y  // intChain to be compared to
); // O(|X|)

// Given two intChains, X and Y, perform X += Y:
void intAdd(
    struct intChain* X, // intChain to be added to
    struct intChain* Y  // intChain to be added
); // O(|X|+|Y|)

// Given two intChains, X and Y, perform X -= Y:
void intSub(
    struct intChain* X, // intChain to be subtracted from
    struct intChain* Y  // intChain to be subtracted
); // O(|X|+|Y|)

// Add one to the given intChain:
void intIncrement(
    struct intChain* X  // intChain to increment
); // O(|X|)

// Subtract one from the given intChain:
void intDecrement(
    struct intChain* X  // intChain to decrement
); // O(|X|)

// Check if the given intChain is even:
uint32_t intIsEven(
    struct intChain* X  // intChain to check the parity of
); // O(1)

// Given two intChains X and Y, reduce X mod Y (much faster than full division):
void intMod(
    struct intChain* X, // intChain to reduce
    struct intChain* Y  // intChain to reduce by
); // O(|X|/|Y|)

// Given two intChains X and Y, reduce X mod Y and return a new intChain containing their quotient:
struct intChain* intDiv(
    struct intChain* X, // intChain to reduce
    struct intChain* Y  // intChain to divide by
); // O(|X|/|Y|)

// Multiply an intChain by a single integer in place:
void intScale(
    struct intChain* X, // intChain to scale
    uint64_t scalar     // integer to scale by
); // O(|X|)

// Return a new intChain containing the product of X and Y:
struct intChain* intMult(
    struct intChain* X, // first intChain to be multiplied
    struct intChain* Y  // second intChain to be multiplied
); // O(|X|*|Y|)

struct intChain* intModExp(
    struct intChain* X, // base
    struct intChain* Y, // exponent
    struct intChain* Z  // modulus
);

// Generate an intChain between zero and X filled with high quality random data:
struct intChain* intCryptoRandom(
    struct intChain* X  // Upper bound for random value
); // Probabilistically O(|X|)

// Generate an intChain between zero and X filled with low quality random data:
//  Faster and less taxing on system resources.
struct intChain* intPseudoRandom(
    struct intChain* X  // Upper bound for random value
); // Probabilistically O(|X|)

// Test whether a given intChain is prime, to confidence 1 - 4^(-security):
//  Uses the Miller-Rabin algorithm.
uint32_t intIsPrime(
    struct intChain* X  // order of magnitude for prime to be generated
);

// Find and return a prime p such that 2^size < p < 2^(size+1):
struct intChain* intMakePrime(
    uint64_t size       // order of magnitude for prime to be generated
);

// Find and return a primitive root mod a prime P:
//  Securely random.
struct intChain* intFindPrimitiveRoot(
    struct intChain* P  // Prime to find a primitive root of
);

struct intChain* intEncodeString(
    char* buffer
);

char* intDecodeString(
    struct intChain* X
);

// Given an intChain X, convert it to a big-endian string:
char* intToString(
    struct intChain* X  // intChain to be converted to a string
); // O(|X|)

// Given a hex-string representing an integer, allocate an intChain capable of holding the represented integer, and fill it with the given data:
struct intChain* intFromString(
    char* buffer        // buffer containing the string to be converted to an intChain
); // O(|buf| + |X|)
