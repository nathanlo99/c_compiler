// test phases: ssa, optimized, interpret
// `a * b` doesn't depend on the loop, but there's no loop-invariant-code-
// motion pass (no natural-loop/back-edge detection at all in the codebase),
// so it's recomputed on every iteration instead of hoisted to a preheader.
int wain(int a, int b) {
  int i = 0;
  int sum = 0;
  while (i < 10) {
    sum = sum + a * b;
    i = i + 1;
  }
  return sum;
}
