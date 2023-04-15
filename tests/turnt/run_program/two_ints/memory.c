
int wain(int a, int value) {
  int *ptr = NULL;
  ptr = new int[1];
  *ptr = *(&value) + 1;
  println(value);
  *(&value) = 2;
  delete[] ptr;
  return value;
}
