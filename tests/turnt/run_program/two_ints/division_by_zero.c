
int wain(int a, int b) {
  if (1 == 0) {
    a = 1 / 0; // Should be a division by zero error
  }
  return a;
}
