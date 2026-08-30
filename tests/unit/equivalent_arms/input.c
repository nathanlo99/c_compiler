// test phases: optimized, interpret
// Both arms of this branch do the exact same thing, so the branch itself
// could become an unconditional jump -- nothing in the optimizer looks for
// this (combine_extended_blocks only contracts a sole single-pred/single-
// succ edge, it has no "both arms converge on equivalent behavior" rule).
int wain(int a, int b) {
  int x = 0;
  if (a == b) {
    x = 1;
  } else {
    x = 1;
  }
  return x;
}
