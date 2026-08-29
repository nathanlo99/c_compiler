
int wain(int a, int b) {
  int *c = NULL;
  c = &a;

  a = 69; // Seemingly unused assignment
  b = *c; // Sneakily used here
  a = 70;

  return a + b; // 69 + 70 = 139
}
