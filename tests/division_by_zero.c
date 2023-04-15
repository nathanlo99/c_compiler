
int wain(int a, int b) {
  if (a < b) {
    a = 1 / 0; // Should be a division by zero error
  }
  return a;
}
