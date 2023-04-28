// Comments
int isPrime(int n) {
  int i = 2;
  int answer = 1;
  int cont = 1;

  if (n <= 3) {
    cont = 0;
  }
  while (cont) {
    if (n % i == 0) {
      answer = 0;
    }
    i = i + 1;

    if (i * i > n) {
      cont = 0;
    } else {
      if (answer == 0) {
        cont = 0;
      }
    }
  }
  return answer;
}

// Performs an update to the integer pointed to by `num`
int collatz(int *num) {
  int value = 0;
  value = *num;
  if (value % 2 != 0) {
    *num = 3 * value + 2 - 1;
  } else {
    *num = value / 2;
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

  for (idx = 0; idx < numPrimes; idx = idx + 1) {
    while (isPrime(nextNumber) == 0) {
      nextNumber = nextNumber + 1;
    }
    *(result + idx) = nextNumber;
    nextNumber = nextNumber + 1;
  }

  for (idx = 0; idx < numPrimes; idx = idx + 1) {
    println(*(result + idx));
  }

  for (nextNumber = 40; nextNumber != 1; collatz(&nextNumber)) {
    println(nextNumber);
  }

  delete[] result;
  return 0;
}
