int toupper(int *a) {
  while (*a != 0) {
    if (*a > 96) {
      if (*a < 123) {
        *a = *a - 32;
      } else {
      }
    } else {
    }
    a = a + 1;
  }
  return 0;
}

int wain(int a, int b) {
  int *c = NULL;
  c = new int[10];
  *c = 97;
  *(c + 1) = 98;
  *(c + 2) = 99;
  *(c + 3) = 100;
  *(c + 4) = 101;
  *(c + 5) = 102;
  *(c + 6) = 103;
  *(c + 7) = 104;
  *(c + 8) = 105;
  *(c + 9) = 0;
  toupper(c);
  delete[] (c);
  return 0;
}
