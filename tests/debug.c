// Comments
int isPrime(int n) {
  int i = 2;
  int answer = 1;
  int cont = 1;

  if (n <= 3) {
    cont = 0;
  }
  while (cont != 0) {
    if (n % i == 0) {
      answer = 0;
      cont = 0;
    }
    i = i + 1;
    if (i >= n) {
      cont = 0;
    }
  }
  return answer;
}

int wain(int n, int unused) {
  return isPrime(n);
}
