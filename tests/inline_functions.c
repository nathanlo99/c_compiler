
// Returns max(a, b)
int p(int a, int b) {
  if (a < b) {
    a = b;
  }
  return a;
}

int q(int a, int b) { return a + b - p(a, b); }

int wain(int a, int b) { return q(a, b); }
