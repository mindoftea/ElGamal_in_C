#define _GNU_SOURCE

#include <stdlib.h>
#include <assert.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/random.h>

#include "intChain.h"

// intChain.c
// Emory Hufbauer, 2016

// Structure for heads of intChains:
struct intChain {
	struct intNode* next;
	struct intNode* last;
	uint64_t size;
};

// Structure for nodes of intChains:
struct intNode {
	struct intNode* next;
	uint64_t data;
};

// Size of ints used in the intNodes:
#define INT_SIZE 64
#define UINT_MAX UINT64_MAX
#define INT_MAX INT64_MAX
#define INT_MIN INT64_MIN

// Self-referential node with value zero for terminating intChains:
static struct intNode rootZero = {&rootZero, 0};

// intChain with value one for convenience functions:
static struct intNode nodeOne = {&rootZero, 1};
static struct intChain chainOne = {&nodeOne, &nodeOne, 1};

// Create a pool to store used intNodes in for recycling:
//  Avoids excessive calls to malloc and free, and their overhead.
#define POOL_SIZE 256
static struct intNode* nodePool[POOL_SIZE];
static int32_t lastInPool = -1;

// Free all the memory left in the pool:
// Because of ((destructor)), this function gets run when the rest of the program finishes.
static void __attribute__((destructor)) clearPool(void) {
	lastInPool++;
	while (lastInPool--) {
		free(nodePool[lastInPool]);
	}
} // O(1)

#ifndef NDEBUG
// Verify that an inChain is properly structured:
static uint32_t intCheck(
    struct intChain* X
) {
	if (!X) {
		return 7;
	} else if (X->next == 0) {
		return 1;
	} else if (X->last == 0 || X->last == &rootZero) {
		return 2;
	} else {
		uint64_t size = 0;
		uint64_t zeroNodeCount = 0;
		struct intNode* currentNode = X->next;
		struct intNode* lastNonZeroNode = (struct intNode*) X;
		while (currentNode != &rootZero) {
			zeroNodeCount++;
			if (currentNode->next == 0 || currentNode->next == &nodeOne) {
				return 3;
			}
			if (currentNode->data) {
				lastNonZeroNode = currentNode;
				zeroNodeCount = 0;
			}
			currentNode = currentNode->next;
			size++;
		}
		if (X->size != size - zeroNodeCount) {
			return 4;
		}
		if (X->last != lastNonZeroNode) {
			return 5;
		}
		if (lastNonZeroNode->next != &rootZero) {
			return 6;
		}
		return 0;
	}
} // O(|X|)
#endif

// Free a headless chain of nodes and return the number of nodes freed:
static uint64_t nodeFree(
    struct intNode* currentNode	// First node to be freed
) {
	assert(currentNode);
	uint64_t numberFreed = 0;
	while (currentNode != &rootZero) {
		struct intNode* nextNode = currentNode->next;
		if (lastInPool + 1 < POOL_SIZE) {
			// If there is space in the recycling pool, put the node there for reuse:
			nodePool[lastInPool + 1] = currentNode;
			lastInPool++;
		} else {
			// If there is no more space in the pool, free the node:
			free(currentNode);
		}
		currentNode = nextNode;
		numberFreed++;
	}
	return numberFreed;
} // O(|currentNode|)

// Free the dynamically allocated data in an intChain:
void intFree(
    struct intChain* X  // intChain to be freed
) {
	assert(!intCheck(X));
	nodeFree(X->next);
	free(X);
} // O(|X|)

// intChain node constructor:
static struct intNode* nodeMake(
    void
) {
	// Using "volatile" prevents the compiler from doing crazy things when optimizing.
	struct intNode* volatile newNode;
	if (lastInPool == -1) {
		// If the recycling pool is empty, allocate a new node:
		newNode = malloc(sizeof * newNode);
		if (!newNode) {
			// If the pool is empty and malloc fails, give up on life:
			exit(1);
		}
	} else {
		// Otherwise, use the last node in the recycling pool:
		newNode = nodePool[lastInPool];
		lastInPool--;
	}
	newNode->data = 0;
	newNode->next = &rootZero;
	// Rootzero is a safer default pointer than NULL.
	return newNode;
} // O(1)

// intChain head constructor:
struct intChain* intMake(
    void
) {
	// Allocate space for the head of the intChain:
	//  Again, using "volatile" prevents the compiler from doing crazy things when optimizing.
	struct intChain* volatile newChain = malloc(sizeof * newChain);
	if (!newChain) {
		// If malloc fails, try clearing the pool to squeeze out a little bit more memory:
		clearPool();
		newChain = malloc(sizeof * newChain);
		if (!newChain) {
			// If it still didn't work, just die:
			exit(1);
		}
	}
	newChain->size = 0;
	newChain->next = &rootZero;
	newChain->last = (struct intNode*) newChain;
	// "Last" always points to the last node before the first rootZero in the chain.
	//  When the chain has length zero, last is the head of the chain itself.
	//  This way, even if the chain has length zero, it is easy to append a node to the end of it.
	return newChain;
} // O(1)

// Given a pointer to an intChain X, create a new intChain which is a copy of X and return a pointer to it:
struct intChain* intCopy(
    struct intChain* X	// intChain to be copied
) {
	assert(!intCheck(X));
	// Create the head of the new intChain:
	struct intChain* Y = intMake();
	struct intNode* currentNodeX = X->next;
	while (currentNodeX != &rootZero) {
		// Make a new node for Y:
		struct intNode* newNodeY = nodeMake();
		newNodeY->data = currentNodeX->data;
		// Append it to the end of Y:
		Y->last->next = newNodeY;
		Y->last = Y->last->next;
		Y->size++;
		currentNodeX = currentNodeX->next;
	}
	return Y;
} // O(|X|)

// Given two intChains, overwrite the data in the first with the data in the second:
//  This allows intChains to be locally reused without the overhead of using the recycling pool.
static struct intChain* intOverwrite(
    struct intChain* X,	// intChain to be overwritten
    struct intChain* Y	// intChain to be copied
) {
	assert(!intCheck(X));
	assert(!intCheck(Y));
	struct intNode* currentNodeX = X->next;
	struct intNode* previousNodeX = (struct intNode*) X;
	struct intNode* currentNodeY = Y->next;
	while (currentNodeY != &rootZero) {
		if (currentNodeX == &rootZero) {
			// If there are no more nodes in X, append a new one:
			currentNodeX = nodeMake();
			previousNodeX->next = currentNodeX;
		}
		// Overwrite the data:
		currentNodeX->data = currentNodeY->data;
		currentNodeY = currentNodeY->next;
		currentNodeX = currentNodeX->next;
		previousNodeX = previousNodeX->next;
	}
	// Free extra nodes in X:
	nodeFree(previousNodeX->next);
	// Update X's head:
	previousNodeX->next = &rootZero;
	X->size = Y->size;
	X->last = previousNodeX;
	return X;
} // O(|Y|)

// Right-shift an intChain X by n 32-bit nodes:
static void intRShiftLarge(
    struct intChain* X,	// intChain to be shifted
    uint64_t n			// Number of nodes to shift it by
) {
	assert(!intCheck(X));
	if (n >= X->size) {
		nodeFree(X->next);
		X->next = &rootZero;
		X->last = (struct intNode*) X;
		X->size = 0;
	}
	struct intNode* currentNode = X->next;
	struct intNode* previousNode = (struct intNode*) X;
	// Proceed to the nth node of X:
	while (n--) {
		previousNode = currentNode;
		currentNode = currentNode->next;
	}
	// Break the chain between the nth and n+1st nodes:
	previousNode->next = &rootZero;
	// Free the first part of the chain (and update the size):
	X->size -= nodeFree(X->next);
	// Reattach the latter part of the chain:
	X->next = currentNode;
} // O(n)

// Right-shift an intChain by n < INT_SIZE bits:
//  This function is performance critical, so I've made some optimizations.
static void intRShiftSmall(
    struct intChain* X,	// intChain to be shifted
    uint32_t n			// Number of bits to shift it by
) {
	assert(!intCheck(X));
	assert(n < INT_SIZE);
	// Shifting by more than INT_SIZE zeroes the int.
	if (X->next == &rootZero) {
		return;
	}
	struct intNode* currentNode = (struct intNode*) X;
	// Proceed through the nodes of X, stopping one node before the end:
	uint64_t k = X->size - 1;
	while (k--) {
		currentNode = currentNode->next;
		// Shift the data in the node to the right by n places:
		currentNode->data >>= n;
		// Add the "underflow" from the next node:
		currentNode->data += currentNode->next->data << (INT_SIZE - n);
	}
	// Note that currentNode now points to the second-to-last node in the intChain.
	// Shift the last node:
	X->last->data >>= n;
	if (X->last->data == 0) {
		// If that last node is now zero, free it:
		nodeFree(X->last);
		X->last = currentNode;
		// Using the pointer to the second-to-last node here.
		currentNode->next = &rootZero;
		X->size--;
	}
} // O(|X|)

// Rightshift an intChain:
//  Wraps intRShiftLarge and intRShiftSmall up together for easy use.
void intRShift(
    struct intChain* X,	// intChain to be shifted
    uint64_t n			// Number of bits to shift it by
) {
	assert(!intCheck(X));
	if (X->next == &rootZero) {
		return;
	}
	if (n / INT_SIZE != 0) {
		// Shift by as many whole nodes as possible:
		intRShiftLarge(X, n / INT_SIZE);
	}
	if (X->next == &rootZero) {
		return;
	}
	if (n % INT_SIZE != 0) {
		// Shift by the leftover amount:
		intRShiftSmall(X, n % INT_SIZE);
	}
} // O(|x| + n)

// Left-shift an intChain by whole nodes:
static void intLShiftLarge(
    struct intChain* X,	// intChain to be shifted
    uint64_t n 			// Number of nodes to shift it by
) {
	assert(!intCheck(X));
	if (X->next == &rootZero) {
		return;
	}
	struct intNode* currentNode = X->next;
	// Construct (in reverse) a chain of n nodes, each with data zero:
	//  The end of the new chain will point to the first node of X.
	while (n--) {
		// Make a new node:
		struct intNode* newNode = nodeMake();
		// Point it to the previous node:
		newNode->next = currentNode;
		currentNode = newNode;
		X->size++;
	}
	// Splice the chain into the beginning of X:
	X->next = currentNode;
} // O(n)

// Left-shift an intChain X by n < INT_SIZE bits:
static void intLShiftSmall(
    struct intChain* X,	// intChain to be shifted
    uint32_t n			// number of bits to shift it by
) {
	assert(!intCheck(X));
	assert(n < INT_SIZE);
	struct intNode* currentNode = (struct intNode*) X;
	uint64_t overflow = 0;
	while (currentNode->next != &rootZero) {
		currentNode = currentNode->next;
		// Save the high bits to be added to the next node:
		uint64_t newOverflow = currentNode->data >> (INT_SIZE - n);
		// Shift the data in the node to the right:
		currentNode->data <<= n;
		// Add the overflow from the previous node:
		currentNode->data += overflow;
		overflow = newOverflow;
	}
	if (overflow) {
		// If necessary, append a new node to the end of the chain:
		currentNode->next = nodeMake();
		currentNode->next->data = overflow;
		X->size++;
		X->last = currentNode->next;
	}
} // O(|X|)

// Leftshift an intChain:
//  Wraps intLShiftLarge and intLShiftSmall up together for easy use.
void intLShift(
    struct intChain* X,	// intChain to be shifted
    uint64_t n			// number of bits to shift it by
) {
	assert(!intCheck(X));
	if (n / INT_SIZE != 0) {
		intLShiftLarge(X, n / INT_SIZE);
	}
	if (n % INT_SIZE != 0) {
		intLShiftSmall(X, n % INT_SIZE);
	}
} // O(|X| + n)

// Returns the index of the highest set bit of an integer:
//  Basically floor(log(x))
static uint32_t highestBitSignificance(
    uint64_t x
)  {
	assert(x != 0);
	// log(0) is borked.
	uint32_t highestSetBit = INT_SIZE;
	// Leftshift x until the highest bit is one:
	while (!(x & 0x8000000000000000)) {
		highestSetBit--;
		x <<= 1;
	}
	// Return the number of times x was leftshifted:
	return highestSetBit;
} // O(1)

// Given two intChains, X and Y, return a heuristic approximation to log(X/Y):
int64_t __attribute__((pure)) intCompare(
    struct intChain* X, // intChain to be compared
    struct intChain* Y  // intChain to be compared to
) {
	assert(!intCheck(X));
	assert(!intCheck(Y));
	if (X->next == &rootZero && Y->next == &rootZero) {
		return 0;
		// log(0/0) --> 0
		// (not quite true, but close enough)
	} else if (X->next == &rootZero) {
		return INT_MIN;
		// log(0/Y) --> -infinity
	} else if (Y->next == &rootZero) {
		return INT_MAX;
		// log(X/0) --> +infinity
	}
	// Start with the difference in significance between the highest node of X, and that of Y:
	int64_t comparitor = INT_SIZE * (X->size - Y->size);
	comparitor += highestBitSignificance(X->last->data);
	comparitor -= highestBitSignificance(Y->last->data);
	return comparitor;
} // O(1)

// Given an intChain, X, return a heuristic approximation to log(X):
int64_t __attribute__((pure)) intMagnitude(
    struct intChain* X  // intChain whose size to estimate
) {
	return intCompare(X, &chainOne);
} // O(1)

// Runs a very fast check to compare two intChains:
//  Returns 1 if the first argument is clearly greater,
//          2 if the second argument is clearly greater,
//      and 0 if they are very similar (but not necessarily equal).
static uint32_t __attribute__((pure)) intHeadCompare(
    struct intChain* X,	// intChain to be compared
    struct intChain* Y	// intChain to be compared to
) {
	assert(!intCheck(X));
	assert(!intCheck(Y));
	// Compare the sizes:
	if (X->size > Y->size) {
		return 1;
	}
	if (Y->size > X->size) {
		return 2;
	}
	// If the sizes were the same, compare the last elements:
	if (X->last->data > Y->last->data) {
		return 1;
	}
	if (Y->last->data > X->last->data) {
		return 2;
	}
	return 0;
} // O(1)

// Given two intChains, X and Y, return 0 if they are equal, 1 if X is greater, and 2 else:
//  Doesn't give false positives, but slow if the arguments are equal or nearly equal.
uint32_t __attribute__((pure)) intFineCompare(
    struct intChain* X,	// intChain to be compared
    struct intChain* Y	// intChain to be compared to
) {
	assert(!intCheck(X));
	assert(!intCheck(Y));
	uint32_t comparitor = intHeadCompare(X, Y);
	if (comparitor) {
		return comparitor;
	}
	struct intNode* currentNodeX = X->next;
	struct intNode* currentNodeY = Y->next;
	// If they are very similar, proceed down the intChains, comparing each node:
	//  Because intHeadCompare already checked the final nodes, stop before the end.
	while (currentNodeX->next != &rootZero) {
		// If one of the nodes is larger, overwrite the comparitor:
		if (currentNodeX->data > currentNodeY->data) {
			comparitor = 1;
		} else if (currentNodeX->data < currentNodeY->data) {
			comparitor = 2;
		}
		currentNodeX = currentNodeX->next;
		currentNodeY = currentNodeY->next;
	}
	return comparitor;
} // O(|X|)

// Given two intChains, X and Y, perform X += Y:
void intAdd(
    struct intChain* X,	// intChain to be added to
    struct intChain* Y	// intChain to be added
) {
	assert(!intCheck(X));
	assert(!intCheck(Y));
	struct intNode* currentNodeX = X->next;
	struct intNode* currentNodeY = Y->next;
	uint32_t carryBit = 0;
	// Proceed until there is nothing left to add:
	while (currentNodeY != &rootZero || carryBit) {
		if (currentNodeX == &rootZero) {
			// If there are no more nodes in X, append a new one:
			currentNodeX = nodeMake();
			X->last->next = currentNodeX;
			X->last = currentNodeX;
			X->size++;
		}
		// Add and check for overflow:
		uint64_t sum = currentNodeX->data + carryBit;
		carryBit = (sum < currentNodeX->data);
		sum += currentNodeY->data;
		carryBit = (sum < currentNodeY->data) + carryBit;
		// Addition is faster than ||.
		currentNodeX->data = sum;
		currentNodeX = currentNodeX->next;
		currentNodeY = currentNodeY->next;
	}
} // O(|X|+|Y|)

// Given two intChains, X and Y, perform X -= Y:
void intSub(
    struct intChain* X,	// intChain to be subtracted from
    struct intChain* Y	// intChain to be subtracted
) {
	assert(!intCheck(X));
	assert(!intCheck(Y));
	struct intNode* currentNodeX = X->next;
	struct intNode* currentNodeY = Y->next;
	struct intNode* lastNonZeroNodeX = (struct intNode*) X;
	uint32_t carryBit = 0;
	// Proceed until there is nothing left to subtract:
	while (currentNodeY != &rootZero || carryBit) {
		// Subtract and check for underflow:
		uint32_t newCarryBit = (currentNodeX->data < currentNodeY->data);
		currentNodeX->data -= currentNodeY->data + carryBit;
		carryBit = newCarryBit + (currentNodeX->data == UINT_MAX);
		// Again, addition for speed.
		if (currentNodeX->data) {
			// Keep track of the last non-zero node:
			lastNonZeroNodeX = currentNodeX;
		}
		currentNodeX = currentNodeX->next;
		currentNodeY = currentNodeY->next;
	}
	if (currentNodeX == &rootZero) {
		// Trim the end of X:
		X->size -= nodeFree(lastNonZeroNodeX->next);
		lastNonZeroNodeX->next = &rootZero;
		X->last = lastNonZeroNodeX;
	}
} // O(|X|+|Y|)

// Add one to the given intChain:
void intIncrement(
    struct intChain* X	// intChain to increment
) {
	intAdd(X, &chainOne);
} // O(|X|)

// Subtract one from the given intChain:
void intDecrement(
    struct intChain* X	// intChain to decrement
) {
	intSub(X, &chainOne);
} // O(|X|)

// Check if the given intChain is even:
uint32_t intIsEven(
    struct intChain* X  // intChain to check the parity of
) {
	return !(X->last->data & 0x0000000000000001);
} // O(1)

// Given two intChains X and Y, reduce X mod Y (much faster than full division):
void intMod(
    struct intChain* X,	// intChain to reduce
    struct intChain* Y	// intChain to reduce by
) {
	assert(!intCheck(X));
	assert(!intCheck(Y));
	// Yell about division by zero:
	assert(Y->next != &rootZero);
	// Use intCompare to get an initial power-of-two scaling factor for Y:
	int64_t exponent = intCompare(X, Y);
	if (exponent < 0) {
		// If the exponent is less than zero, Y>X, so return.
		return;
	}
	// Safety factor:
	exponent += 2;
	// Shift Y by exponent bits:
	intLShift(Y, (uint64_t) exponent);
	// For all powers of two less than 2^exponent:
	while (exponent--) {
		intRShiftSmall(Y, 1);
		if (intFineCompare(X, Y) < 2) {
			// If X >= Y, subtract Y from X:
			intSub(X, Y);
		}
	}
	// Note that Y is now restored to its original value.
} // O(|X|/|Y|)

// Given two intChains X and Y, reduce X mod Y and return a new intChain containing their quotient:
struct intChain* intDiv(
    struct intChain* X,	// intChain to reduce
    struct intChain* Y	// intChain to divide by
) {
	assert(!intCheck(X));
	assert(!intCheck(Y));
	// Yell about division by zero:
	assert(Y->next != &rootZero);
	if (X->next == &rootZero) {
		// If X is zero, return zero:
		return intMake();
	}
	// Use intCompare to get an initial power-of-two scaling factor for Y:
	int64_t exponent = intCompare(X, Y);
	if (exponent < 0) {
		// If the exponent is less than zero, Y>X, so return.
		return intMake();
	}
	struct intChain* quotient = intMake();
	struct intChain* currentPower = intCopy(&chainOne);
	// Safety factor
	exponent += 2;
	intLShift(currentPower, (uint64_t) exponent);
	intLShift(Y, (uint64_t) exponent);
	while (exponent--) {
		intRShift(Y, 1);
		intRShift(currentPower, 1);
		if (intFineCompare(X, Y) < 2) {
			// If X >= Y, subtract Y from X:
			intSub(X, Y);
			// Increment the quotient:
			intAdd(quotient, currentPower);
		}
	}
	intFree(currentPower);
	return quotient;
} // O(|X|/|Y|)

// Multiply an intChain by a single integer in place:
void intScale(
    struct intChain* X,	// intChain to scale
    uint64_t scalar		// integer to scale by
) {
	assert(!intCheck(X));
	if (scalar == 0) {
		nodeFree(X->next);
		X->next = &rootZero;
		X->last = (struct intNode*) X;
		X->size = 0;
		return;
	} else if (X->next == &rootZero || scalar == 1) {
		return;
	}
	struct intNode* currentNode = X->next;
	// Split the scalar into a low and high part, each of half size:
	uint64_t scalarLow = (uint32_t)(scalar);
	uint64_t scalarHigh = scalar >> INT_SIZE / 2;
	// Proceed until there are no more nodes in X, and there is nothing left to carry:
	uint64_t carry = 0;
	while (currentNode != &rootZero || carry) {
		if (currentNode == &rootZero) {
			// If there are no more nodes in X, append a new one:
			currentNode = nodeMake();
			X->last->next = currentNode;
			X->last = currentNode;
			X->size++;
		}
		// Split the data from the current node into a low and high part:
		uint64_t nodeDataLow = (uint32_t)(currentNode->data);
		uint64_t nodeDataHigh = currentNode->data >> INT_SIZE / 2;
		// Multiply in parts:
		uint64_t lowTerm = nodeDataLow * scalarLow;
		uint64_t middleTerm = nodeDataLow * scalarHigh + nodeDataHigh * scalarLow;
		uint64_t highTerm = nodeDataHigh * scalarHigh;
		// Set the currentNode's data to the low part of the middle term:
		currentNode->data = (middleTerm << INT_SIZE / 2);
		// Add the carry:
		currentNode->data += carry;
		// Check for overflow on that addition:
		carry = (currentNode->data < carry);
		// Add the lowterm to the currentNode's data and check for overflow:
		currentNode->data += lowTerm;
		carry += (currentNode->data < lowTerm);
		// Check for overflow on the middleTerm:
		// Shift left because the middleTerm has higher order than the low term.
		carry += (uint64_t)(middleTerm < nodeDataLow * scalarHigh) << INT_SIZE / 2;
		// Carry the high part of the middleTerm on to the next node:
		carry += middleTerm >> INT_SIZE / 2;
		// Add the highterm:
		carry += highTerm;
		currentNode = currentNode->next;
	}
} // O(|X|)

// Return a new intChain containing the product of X and Y:
struct intChain* intMult(
    struct intChain* X, // First intChain to be multiplied
    struct intChain* Y	// Second intChain to be multiplied
) {
	assert(!intCheck(X));
	assert(!intCheck(Y));
	if (Y->next == &rootZero || X->next == &rootZero) {
		// Multiplication by zero:
		return intMake();
	}
	struct intChain* Product = intMake();
	struct intChain* Component = intMake();
	// For all nodes in X:
	uint64_t significance = 0;
	struct intNode* currentNodeX = X->next;
	while (currentNodeX != &rootZero) {
		// Copy Y onto the Component:
		intOverwrite(Component, Y);
		// Scale the Component by the data in the current node of X:
		intScale(Component, currentNodeX->data);
		// Shift it by the significance of the current node of X:
		intLShiftLarge(Component, significance++);
		// Add it to the Product:
		intAdd(Product, Component);
		currentNodeX = currentNodeX->next;
	}
	intFree(Component);
	return Product;
} // O(|X|*|Y|)

struct intChain* intModExp(
    struct intChain* X,	// base
    struct intChain* Y,	// exponent
    struct intChain* Z	// modulus
) {
	assert(!intCheck(X));
	assert(!intCheck(Y));
	assert(!intCheck(Z));
	struct intChain* W = intCopy(X);
	// X to a power of two.
	struct intChain* Result = intCopy(&chainOne);
	// For every node in Y:
	struct intNode* currentNodeY = Y->next;
	while (currentNodeY != &rootZero) {
		// For every bit in the node's data:
		uint64_t nodeBits = currentNodeY->data;
		uint32_t bitCounter = INT_SIZE;
		while (bitCounter--) {
			// Reduce mod Z for efficiency:
			intMod(W, Z);
			if (nodeBits & 0x1) {
				// Reduce for efficiency:
				intMod(Result, Z);
				// If the current power of two is in the binary representation of Y, multiply Result by W:
				struct intChain* Product = intMult(Result, W);
				intFree(Result);
				Result = Product;
			}
			// Square W:
			struct intChain* Product = intMult(W, W);
			intFree(W);
			W = Product;
			nodeBits >>= 1;
		}
		currentNodeY = currentNodeY->next;
	}
	intFree(W);
	intMod(Result, Z);
	return Result;
}

// Generate an intChain of size k filled with high quality random data:
static struct intChain* intCryptoRandomNodes(
    uint64_t k		// size of intChain to return
) {
	struct intChain* Y = intMake();
	struct intNode* lastNonZeroNode = (struct intNode*) Y;
	while (k--) {
		volatile uint8_t randomData[INT_SIZE / 4];
		// Volatile prevents the compiler from doing crazy things.
		syscall(SYS_getrandom, &randomData, INT_SIZE / 4, 0);
		// Reads from dev/urandom on Linux.
		uint32_t j = INT_SIZE / 8;
		uint64_t newData = 0;
		while (j--) {
			// Load the random bytes into a long int:
			newData <<= 8;
			newData += randomData[j];
		}
		// Put that data into a new node:
		struct intNode* newNode = nodeMake();
		newNode->data = newData;
		Y->last->next = newNode;
		Y->last = newNode;
		Y->size++;
		if (newData != 0) {
			lastNonZeroNode = newNode;
		}
	}
	Y->size -= nodeFree(lastNonZeroNode->next);
	Y->last = lastNonZeroNode;
	lastNonZeroNode->next = &rootZero;
	return Y;
} // O(k)

// Generate an intChain between zero and X filled with high quality random data:
struct intChain* intCryptoRandom(
    struct intChain* X	// Upper bound for random value
) {
	assert(!intCheck(X));
	struct intChain* Y = intCryptoRandomNodes(X->size);
	// Discard the unneeded high bits:
	intRShift(Y, INT_SIZE - highestBitSignificance(X->last->data));
	intMod(Y, X);
	return Y;
} // Probabilistically O(|X|)

// Generate an intChain between zero and X filled with low quality random data:
//  Faster and less taxing on system resources.
struct intChain* intPseudoRandom(
    struct intChain* X	// Upper bound for random value
) {
	assert(!intCheck(X));
	struct intChain* Y = intMake();
	struct intNode* lastNonZeroNode = (struct intNode*) Y;
	uint32_t RAND_WIDTH = highestBitSignificance(RAND_MAX);
	uint64_t k = X->size;
	while (k--) {
		uint32_t j = (INT_SIZE / RAND_WIDTH) + (INT_SIZE % RAND_WIDTH);
		uint64_t newData = 0;
		while (j--) {
			// Load the random bytes into a long int:
			newData <<= RAND_WIDTH;
			newData += rand();
		}
		// Put that data into a new node:
		struct intNode* newNode = nodeMake();
		newNode->data = newData;
		Y->last->next = newNode;
		Y->last = newNode;
		Y->size++;
		if (newData != 0) {
			lastNonZeroNode = newNode;
		}
	}
	Y->size -= nodeFree(lastNonZeroNode->next);
	Y->last = lastNonZeroNode;
	lastNonZeroNode->next = &rootZero;
	intMod(Y, X);
	if (intMagnitude(Y) < 4) {
		return intPseudoRandom(X);
		// If the result is ridiculously small, something went wrong.
	}
	return Y;
} // Probabilistically O(|X|)

// Number of iterations to use with the prime finding algorithm below:
#define PRIME_CONFIDENCE 50

// Test whether a given intChain is prime, to confidence 1 - 4^(-confidence):
//  Uses the Miller-Rabin algorithm.
uint32_t intIsPrime(
    struct intChain* X	// potential prime to be tested
) {
	struct intChain* XMinusOne = intCopy(X);
	intDecrement(XMinusOne);
	struct intChain* XMinusTwo = intCopy(XMinusOne);
	intDecrement(XMinusTwo);
	uint32_t twoExponent = 0;
	struct intChain* OddPart = intCopy(XMinusOne);
	while (intIsEven(OddPart)) {
		intRShift(OddPart, 1);
		twoExponent++;
	}
	uint64_t confidence = PRIME_CONFIDENCE;
MAYBE_PRIME:
	while (confidence--) {
		struct intChain* Witness = intPseudoRandom(XMinusTwo);
		struct intChain* Swap = intModExp(Witness, OddPart, X);
		intFree(Witness);
		Witness = Swap;
		if (intFineCompare(Witness, &chainOne) == 0 || intFineCompare(Witness, XMinusOne) == 0) {
			intFree(Witness);
			goto MAYBE_PRIME;
		}
		uint32_t currentExponent = twoExponent;
		while (currentExponent--) {
			Swap = intMult(Witness, Witness);
			intMod(Swap, X);
			intFree(Witness);
			Witness = Swap;
			// The below "optimization" actually destroys the cryptosystem! It is left here for posterity
			//  Many thanks to the esteemed Professor Klapper of the Univeristy of Kentucky for pointing this out!
			/* if (intFineCompare(Witness, &chainOne) == 0) {
				intFree(Witness);
				goto NOT_PRIME;
			} else */
			if (intFineCompare(Witness, XMinusOne) == 0) {
				intFree(Witness);
				goto MAYBE_PRIME;
			}
		}
		intFree(Witness);
		goto NOT_PRIME;
	}
	intFree(XMinusOne);
	intFree(XMinusTwo);
	intFree(OddPart);
	return 1;
NOT_PRIME:
	intFree(XMinusOne);
	intFree(XMinusTwo);
	intFree(OddPart);
	return 0;
}

// Find and return a prime p such that 2^size < p <= 2^(size+1):
//  Securely random.
struct intChain* intMakePrime(
    uint64_t size		// order of magnitude for prime to be generated
) {
	assert(size > 2);
	struct intChain* LowerBound = intMake();
	intIncrement(LowerBound);
	intLShift(LowerBound, size - 1);
	// LowerBound is now 2^size.
	struct intChain* UpperBound = intCopy(LowerBound);
	intLShift(UpperBound, 1);
	// UpperBound is now 2^(size+1).
	struct intChain* X = intCryptoRandom(LowerBound);
	intLShift(X, 1);
	intIncrement(X);
	uint32_t isRandom = 1;
	while (
	    intFineCompare(LowerBound, X) != 2 ||
	    intFineCompare(X, UpperBound) != 2 ||
	    !intIsPrime(X)
	) {
		if (isRandom) {
			intLShift(X, 1);
			intIncrement(X);
			intMod(X, UpperBound);
			isRandom = 0;
		} else {
			intFree(X);
			X = intCryptoRandom(LowerBound);
			intLShift(X, 1);
			intIncrement(X);
			isRandom = 1;
		}
	}
	intFree(LowerBound);
	intFree(UpperBound);
	return X;
}

// Find and return a primitive root mod a prime P:
//  Securely random.
struct intChain* intFindPrimitiveRoot(
    struct intChain* P	// Prime to find a primitive root of
) {
	assert(!intCheck(P));
	assert(intIsPrime(P));
	struct intChain* Phi = intCopy(P);
	intDecrement(Phi);
	struct intChain* G = intCryptoRandom(Phi);
	struct intChain* W = intModExp(G, Phi, P);
	while (intCompare(&chainOne, W)) {
		intFree(G);
		intFree(W);
		G = intCryptoRandom(P);
		W = intModExp(G, Phi, P);
	}
	intFree(Phi);
	intFree(W);
	return G;
}

// Encode a string 
struct intChain* intEncodeString(
    char* buffer
) {
	assert(buffer);
	struct intChain* X = intMake();
	struct intNode* lastNonZeroNode = (struct intNode*) X;
	uint32_t stillRunning = 1;
	while (stillRunning) {
		struct intNode* newNode = nodeMake();
		X->last->next = newNode;
		X->last = newNode;
		X->size++;
		uint32_t j = INT_SIZE / 8;
		while (j--) {
			uint32_t newData = *(buffer++);
			if (newData == 0) {
				stillRunning = 0;
				newNode->data <<= 8 * (j + 1);
				break;
			}
			newNode->data <<= 8;
			newNode->data += newData;
		}
		if (newNode->data != 0) {
			lastNonZeroNode = newNode;
		}
	}
	X->size -= nodeFree(lastNonZeroNode->next);
	X->last = lastNonZeroNode;
	lastNonZeroNode->next = &rootZero;
	return X;
}

char* intDecodeString(
    struct intChain* X	// Prime to find a primitive root of
) {
	assert(!intCheck(X));
	char* buffer = malloc((X->size + 1) * INT_SIZE / 8 + 1);
	char* bufferLoc = buffer;
	struct intNode* currentNode = X->next;
	while (currentNode != &rootZero) {
		uint64_t data = currentNode->data;
		uint32_t j = INT_SIZE / 8;
		while (j--) {
			*(bufferLoc++) = (unsigned char)(data >> 8 * (INT_SIZE / 8 - 1));
			data <<= 8;
		}
		currentNode = currentNode->next;
	}
	*(bufferLoc++) = 0;
	return buffer;
}

// Given an intNode, recursively convert it and subsequent nodes into a little-endian hex string; return a pointer to the end of the string:
static char* textify(
    struct intNode* currentNode,
    char* buffer
) {
	assert(currentNode && buffer);
	if (currentNode == &rootZero) {
		return buffer;
	}
	uint64_t data = currentNode->data;
	uint32_t currentNibble = 16;
	*(buffer++) = ' ';
	while (currentNibble--) {
		char currentDigit = data & 0x0000000F;
		if (currentDigit < 10) {
			currentDigit += 48;
		} else {
			currentDigit += 55;
		}
		*(buffer++) = currentDigit;
		data >>= 4;
	}
	return textify(currentNode->next, buffer);
} // O(|currentNode|)

// Given an intChain X, convert it to a big-endian string:
char* intToString(
    struct intChain* X
) {
	assert(!intCheck(X));
	char* buffer;
	if (X->next == &rootZero) {
		buffer = malloc(2);
		assert(buffer);
		buffer[0] = '0';
		buffer[1] = 0;
		return buffer;
	}
	buffer = malloc(17 * (X->size));
	assert(buffer);
	char* swapRight = textify(X->next, buffer) - 1;
	char* swapLeft = buffer;
	*(swapLeft) = 0;
	while (swapLeft < swapRight) {
		char swapChar = *swapLeft;
		*swapLeft = *swapRight;
		*swapRight = swapChar;
		swapLeft++;
		swapRight--;
	}
	return buffer;
} // O(|X|)

// Given a hex-string representing an integer, allocate an intChain capable of holding the represented integer, and fill it with the given data:
struct intChain* intFromString(
    char* buffer
) {
	assert(buffer);
	struct intChain* X = intMake();
	struct intNode* currentNode = &rootZero;
	struct intNode* lastNode = (struct intNode*) X;
	uint64_t nodeCount = 0;
	uint32_t currentNibble = 0;
	char* currentBufferLocation = buffer;
	while (*currentBufferLocation != 0) {
		char currentDigit = *(currentBufferLocation++);
		if (currentDigit >= 48 && currentDigit <= 57) {
			currentDigit -= 48;
		} else if (currentDigit >= 65 && currentDigit <= 70) {
			currentDigit -= 55;
		} else {
			continue;
		}
		if (lastNode == (struct intNode*) X && currentDigit == 0) {
			continue;
		}
		if (currentNibble == 0) {
			struct intNode* newNode = nodeMake();
			newNode->next = currentNode;
			currentNode = newNode;
			if (lastNode == (struct intNode*) X) {
				lastNode = currentNode;
			}
			currentNibble = INT_SIZE / 4;
			nodeCount++;
		}
		currentNibble--;
		currentNode->data += (uint64_t) currentDigit << 4 * currentNibble;
	}
	X->next = currentNode;
	X->last = lastNode;
	X->size = nodeCount;
	intRShift(X, 4 * currentNibble);
	return X;
} // O(|buf| + |X|)

