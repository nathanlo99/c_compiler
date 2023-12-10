
int factorial(int a) {
  int result = 0;
  if (a <= 1) {
    result = 1;
  } else {
    result = a * factorial(a - 1);
  }
  return result;
}

int wain(int a, int result) {
  result = factorial(a);
  return result;
}
