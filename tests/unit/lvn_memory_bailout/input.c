// test phases: ssa, optimized, interpret
// LVN bails on this ENTIRE block just because it contains a Store, even
// though the redundant arithmetic below has nothing to do with memory at
// all: `y` should be a copy of `x` (same-block CSE of `a + b`), but never
// gets recognized as such purely because of the unrelated `*p = 5;` in the
// same block. No aliasing/ordering question here at all (unlike item 8) --
// this is item 6's exact mechanism, isolated.
int wain(int a, int b) {
  int *p = NULL;
  int x = 0;
  int y = 0;
  p = new int[1];
  *p = 5;
  x = a + b;
  y = a + b;
  delete[] p;
  return x + y;
}
