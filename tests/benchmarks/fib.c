
int fib(int n) {
  int answer = 0;
  if (n <= 2) {
    answer = 1;
  } else {
    answer = fib(n - 1) + fib(n - 2);
  }
  return answer;
}

int wain(int a, int b) {
  b = fib(a);
  println(b);
  return b;
}
