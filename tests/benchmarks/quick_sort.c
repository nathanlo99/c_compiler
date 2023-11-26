int partition(int *array, int left, int right) {
  int pivot = 0;
  int i = 0;
  int j = 0;
  int temp = 0;
  int ret = 0;
  pivot = *(array + left);
  i = left - 1;
  j = right + 1;
  while (i < j) {
    i = i + 1;
    while (*(array + i) < pivot) {
      i = i + 1;
    }
    j = j - 1;
    while (*(array + j) > pivot) {
      j = j - 1;
    }
    temp = *(array + i);
    *(array + i) = *(array + j);
    *(array + j) = temp;
  }
  return ret;
}
int fastSort(int *array, int left, int right) {
  int skip = 0;
  int pivot = 0;
  if (left >= 0 && right >= 0 && left < right) {
    pivot = partition(array, left, right);
    skip = fastSort(array, left, pivot);
    skip = fastSort(array, pivot + 1, right);
  }
  return 0;
}

int wain(int *a, int b) {
  int i = 0;
  for (i = 0; i < b; i = i + 1) {
    println(*(a + i));
  }
  fastSort(a, 0, b - 1);
  for (i = 0; i < b; i = i + 1) {
    println(*(a + i));
  }
  return 0;
}
