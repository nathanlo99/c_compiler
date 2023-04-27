
int wain(int a, int value) {
  int *ptr = NULL;
  ptr = &a;
  *ptr = *(&value) + 1;
  *(&value) = 2;
  return *ptr;
}
