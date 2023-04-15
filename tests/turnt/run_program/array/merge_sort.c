
int merge(int *arr, int left, int mid, int right) {
  int i = 0;
  int j = 0;
  int k = 0;
  int n1 = 0;
  int n2 = 0;
  int *L = NULL;
  int *R = NULL;
  int cont = 1;
  n1 = mid - left + 1;
  n2 = right - mid;
  L = new int[n1];
  R = new int[n2];
  i = 0;
  while (i < n1) {
    *(L + i) = *(arr + left + i);
    i = i + 1;
  }
  j = 0;
  while (j < n2) {
    *(R + j) = *(arr + mid + 1 + j);
    j = j + 1;
  }
  i = 0;
  j = 0;
  k = left;
  while (cont == 1) {
    if (*(L + i) <= *(R + j)) {
      *(arr + k) = *(L + i);
      i = i + 1;
    } else {
      *(arr + k) = *(R + j);
      j = j + 1;
    }
    k = k + 1;
    if (i >= n1) {
      cont = 0;
    }
    if (j >= n2) {
      cont = 0;
    }
  }
  while (i < n1) {
    *(arr + k) = *(L + i);
    i = i + 1;
    k = k + 1;
  }
  while (j < n2) {
    *(arr + k) = *(R + j);
    j = j + 1;
    k = k + 1;
  }
  delete[] L;
  delete[] R;
  return 0;
}

int merge_sort(int *arr, int left, int right) {
  int mid = 0;
  int unused = 0;
  if (left < right) {
    mid = left + (right - left) / 2;
    unused = merge_sort(arr, left, mid);
    unused = merge_sort(arr, mid + 1, right);
    unused = merge(arr, left, mid, right);
  }
  return 0;
}

int wain(int *a, int b) {
  int i = 0;
  int unused = 0;
  while (i < b) {
    println(*(a + i));
    i = i + 1;
  }
  unused = merge_sort(a, 0, b - 1);
  i = 0;
  while (i < b) {
    println(*(a + i));
    i = i + 1;
  }
  return 0;
}
