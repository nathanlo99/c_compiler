
int wain(int *arr, int length) {
  int idx = 0;
  while (idx < length) {
    println(*(arr + idx));
    idx = idx + 1;
  }
  return 0;
}
