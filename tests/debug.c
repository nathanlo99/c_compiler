
int merge(int *arr, int left) {
  int i = 0;
  int j = 0;
  int k = 0;
  int n1 = 0;
  int n2 = 0;
  int *L = NULL;
  int *R = NULL;
  int cont = 1;
  n1 = 4 - left;
  n2 = 1;
  L = new int[n1];
  R = new int[1];
  j = 0;
  while (j < n2) {
    *(R + j) = *(arr + 3 + j);
    j = j + 1;
  }
  i = 0;
  j = 0;
  k = left;
  while (cont == 1) {
    println(k);
    cont = 0;
    if (*(L + i) <= *(R + j)) {
      *(arr + k) = *(L + i);
      i = i + 1;
    } else {
      *(arr + k) = *(R + j);
      j = j + 1;
    }
  }
  return 0;
}

int printArray(int *arr, int size) {
  int i = 0;
  println(10000);
  while (i < size) {
    println(*(arr + i));
    i = i + 1;
  }
  println(10001);
  return 0;
}

int wain(int unused, int b) {
  int* a = NULL;
  a = new int[4];
  *(a + 0) = 1;
  *(a + 1) = 4;
  *(a + 2) = 2;
  *(a + 3) = 3;
  unused = merge(a, 2);
  unused = printArray(a, 4); // 1 4 2 3
  return 0;
}
