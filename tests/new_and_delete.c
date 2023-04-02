
int wain(int *arr, int size) {
  int idx = 0;
  arr = new int[size];

  idx = 0;
  while (idx < size) {
    *(arr + idx) = idx * idx;
    idx = idx + 1;
  }

  idx = 0;
  while (idx < size) {
    println(*(arr + idx));
    idx = idx + 1;
  }

  delete[] arr;
  return 0;
}
