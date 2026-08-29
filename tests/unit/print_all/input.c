
int wain(int *arr, int length) {
  int idx = 0;
  int sum = 0;
  while (idx < length) {
    println(*(arr + idx));
    sum = sum + *(arr + idx);
    idx = idx + 1;
  }
  return sum;
}
