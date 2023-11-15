
int square(int n) { return n * n; }
int cube(int n) { return n * n * n; }

int max(int a, int b) {
  if (a < b) {
    a = b;
  }
  return a;
}

int min(int a, int b) { return a + b - max(a, b); }

int sqrt(int n) {
  int ans = 0;
  while (ans * ans < n) {
    ans = ans + 1;
  }
  return ans;
}

int pythagoras(int a, int b) {
  return sqrt(square(min(a, b)) + square(max(a, b)));
}

int wain(int a, int b) { return cube(pythagoras(3, 4)); }
