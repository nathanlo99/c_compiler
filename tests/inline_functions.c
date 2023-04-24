
int square(int n) { return n * n; }
int cube(int n) { return n * n * n; }

int max(int a, int b) {
  if (a < b) {
    a = b;
  }
  return a;
}

int min(int a, int b) { return a + b - max(a, b); }
int pythagoras(int a, int b) { return square(min(a, b)) + square(max(a, b)); }

int wain(int a, int b) { return pythagoras(3, 4); }
