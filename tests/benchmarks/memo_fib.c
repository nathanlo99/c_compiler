
int fib(int n) {
  int *memo = NULL;
  int i = 0;
  int res = 0;

  memo = new int[n + 1];
  *memo = 1;
  *(memo + 1) = 1;
  for (i = 2; i <= n; i = i + 1) {
    *(memo + i) = *(memo + (i - 1)) + *(memo + (i - 2));
  }
  res = *(memo + n);
  delete[] memo;
  return res;
}

int wain(int n, int res) {
  res = fib(n);
  println(res);
  return res;
}
