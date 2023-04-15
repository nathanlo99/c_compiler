// Comments
int is_prime(int n) {
  int i = 2;
  int answer = 1;
  int continueLooping = 1;

  if (n <= 3) {
    continueLooping = 0;
  }
  while (continueLooping) {
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
  if (value % 2 != 0) {
    *num = 3 * value + 1;
  } else {
    *num = value / 2;
  }
  return 0;
}

// Computes the first [numPrimes] primes, starting at [startNumber]
// Then, prints the following Collatz sequence:
// 40, 20, 10, 5, 16, 8, 4, 2, 1
int wain(int num_primes, int start_number) {
  int *result = NULL;
  int idx = 0;
  int next_number = 0;
  result = new int[num_primes];
  next_number = start_number;

  while (idx < num_primes) {
    while (is_prime(next_number) == 0) {
      next_number = next_number + 1;
    }
    *(result + idx) = next_number;
    next_number = next_number + 1;
  }

  idx = 0;
  while (idx < num_primes) {
    println(*(result + idx));
  }

  next_number = 40;
  while (next_number != 1) {
    println(next_number);
    collatz(&next_number);
  }

  delete[] result;
  return 0;
}
