int partition(int *array, int left, int right) {
  int pivot = 0;
  int i = 0;
  int j = 0;
  int temp = 0;
  int done = 0;
  int ret = 0;
  pivot = *(array + left);
  i = left - 1;
  j = right + 1;
  while (done == 0) {
    i = i + 1;
    while (*(array + i) < pivot) {
      i = i + 1;
    }
    j = j - 1;
    while (*(array + j) > pivot) {
      j = j - 1;
    }
    if (i >= j) {
      ret = j;
      done = 241;
    }
    if (done != 241) {
      temp = *(array + i);
      *(array + i) = *(array + j);
      *(array + j) = temp;
    }
  }
  return ret;
}
int fastSort(int *array, int left, int right) {
  int skip = 0;
  int pivot = 0;
  if (left < 0) {
    skip = 1;
  }
  if (right < 0) {
    skip = 1;
  }
  if (right <= left) {
    skip = 1;
  }
  if (skip == 0) {
    pivot = partition(array, left, right);
    skip = fastSort(array, left, pivot);
    skip = fastSort(array, pivot + 1, right);
  }
  return 0;
}

int wain(int *a, int b) {
  int sort = 0;
  int i = 0;
  while (i < b) {
    println(*(a + i));
    i = i + 1;
  }
  sort = fastSort(a, 0, b - 1);
  i = 0;
  while (i < b) {
    println(*(a + i));
    i = i + 1;
  }
  return 0;
}
