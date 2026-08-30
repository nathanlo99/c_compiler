// test phases: optimized, interpret
// A store that's immediately overwritten with no intervening read is dead,
// but nothing in the optimizer ever removes a Store instruction (see the
// "TODO: Figure out what to do if a memory access / write happens" in
// dead_code_elimination.cpp).
int wain(int a, int b) {
  int *p = NULL;
  int x = 0;
  p = new int[1];
  *p = 1;
  *p = 2;
  x = *p;
  delete[] p;
  return x;
}
