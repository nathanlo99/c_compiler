
// Compute max(a, b)
int wain(int a, int b) {
  int *c = NULL;

  if (a > b) {
    c = &a;
  } else {
    c = &b;
  }

  return *c;
}
