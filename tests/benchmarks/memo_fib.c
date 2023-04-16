
int fib(int *memo, int n) {
  int i = 0;
  while (i <= n) {
    if (i <= 2) {
      *(memo + i) = 1;
    } else {
      *(memo + i) = *(memo + (i - 1)) + *(memo + (i - 2));
    }
    i = i + 1;
  }
  return *(memo + n);
}

int wain(int a, int b) {
  int *memo = NULL;
  memo = new int[a + 1];
  b = fib(memo, a);
  println(b);
  delete[] memo;
  return b;
}
