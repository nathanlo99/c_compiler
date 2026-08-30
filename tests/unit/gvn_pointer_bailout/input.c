// test phases: optimized, interpret
// GVN bails out of the ENTIRE function the moment it touches a pointer
// (see global_value_numbering.hpp's `function.uses_pointers()` guard), not
// just around the memory op -- so even this straight-line redundant load
// with no intervening store, no aliasing ambiguity at all, never gets
// deduplicated.
int wain(int a, int b) {
  int *p = NULL;
  int x = 0;
  int y = 0;
  p = new int[1];
  *p = 1;
  x = *p;
  y = *p;
  delete[] p;
  return x + y;
}
