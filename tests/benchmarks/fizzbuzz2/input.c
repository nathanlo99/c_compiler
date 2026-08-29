
int fizzbuzz(int n) {
  if (n % 15 == 0) {
    return 0 - 15;
  }
  if (n % 3 == 0) {
    return 0 - 3;
  }
  if (n % 5 == 0) {
    return 0 - 5;
  }
  return n;
}

int wain(int start, int end) {
  int idx = 0;
  for (idx = start; idx < end; idx = idx + 1) {
    println(fizzbuzz(idx));
  }
  return idx;
}
