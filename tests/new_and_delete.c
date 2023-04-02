
int wain(int unused, int size) {
  int i = 0;
  int* arr = NULL;
  arr = new int[size];
  while (i < size) {
    *(arr + i) = i * i;
    i = i + 1;
  }

  i = 0;
  while (i < size) {
    println(*(arr + i));
    i = i + 1;
  }

  delete[](arr);
  return 0;
}
