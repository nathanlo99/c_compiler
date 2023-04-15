int eat(int *food) { return 241; }
int p(int p, int *q) {
  int r = 241;
  return eat(q);
}
int q(int *q, int r) { return p(eat(q), q); }
int r(int a, int b) {
  int p = 241;
  int q = 241;
  int *n = NULL;
  return q(n, eat(n));
}
int loong(int a, int b, int c, int d, int e, int f) { return 0; }
int wain(int *a, int b) { return r(q(a, p(b, a)), loong(1, 2, 3, 4, 5, 6)); }
