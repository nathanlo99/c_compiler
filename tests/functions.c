
int f(int a, int b) {
  int c = 0;
  int* d = NULL;
  d = &b;
  d = &c;
  return *d;
}

int wain(int a, int b) {
  return f(a, b);
}
