
// Computes the first [numPrimes] primes, starting at [startNumber]
int wain(int n, int i) {
  int answer = 1;
  int cont = 1;

  while (cont != 0) {
    if (n % i == 0) {
      answer = 0;
    }
    i = i + 1;

    if (i >= n) {
      cont = 0;
    } else {
      if (answer == 0) {
        cont = 0;
      }
    }
  }
  return answer;
}
