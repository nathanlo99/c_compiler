int f(int a) {
  int result = 0;
  if (a == 0) {
    result = 0;
  } else {
    result = f(a - 1);
  }
  return result;
}

int wain(int a, int b) { return f(f(f(100000))); }
