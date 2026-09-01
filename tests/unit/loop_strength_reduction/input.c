// test phases: ssa, optimized, interpret
// `i * 4` is a classic induction-variable strength-reduction candidate: `i`
// increases by 1 each iteration, so `i * 4` increases by a constant 4 each
// iteration and the multiply could become an accumulator add. Nothing does
// this (no loop analysis exists at all), so the multiply survives.
int wain(int a, int b) {
  int i = 0;
  int result = 0;
  while (i < 10) {
    result = result + i * 4;
    i = i + 1;
  }
  return result;
}
