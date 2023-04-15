// Comments
int is_prime(int n) {
  int i = 2;
  int answer = 1;
  int continueLooping = 1;

  if (n <= 3) {
    continueLooping = 0;
  }
  while (continueLooping != 0) {
    if (n % i == 0) {
      answer = 0;
    }
    i = i + 1;

    if (i * i > n) {
      continueLooping = 0;
    } else {
      if (answer == 0) {
        continueLooping = 0;
      }
    }
  }
  return answer;
}

// Performs an update to the integer pointed to by `num`
int collatz(int *num) {
  int value = 0;
  value = *num;
  if (value >= 2) {
    if (value % 2 != 0) {
      *num = 3 * value + 2 - 1;
    } else {
      *num = value / 2;
    }
  } else {
    *num = 1;
  }
  return 0;
}

// Computes the first [numPrimes] primes, starting at [startNumber]
// Then, prints the following Collatz sequence:
// 40, 20, 10, 5, 16, 8, 4, 2
int wain(int startNumber, int numPrimes) {
  int *result = NULL;
  int idx = 0;
  int nextNumber = 0;
  result = new int[numPrimes];
  nextNumber = startNumber;

  while (idx < numPrimes) {
    while (is_prime(nextNumber) == 0) {
      nextNumber = nextNumber + 1;
    }
    *(result + idx) = nextNumber;
    nextNumber = nextNumber + 1;
    idx = idx + 1;
  }

  idx = 0;
  while (idx < numPrimes) {
    println(*(result + idx));
    idx = idx + 1;
  }

  nextNumber = 40;
  while (nextNumber != 1) {
    println(nextNumber);
    idx = collatz(&nextNumber);
  }

  delete[] result;
  return 0;
}
