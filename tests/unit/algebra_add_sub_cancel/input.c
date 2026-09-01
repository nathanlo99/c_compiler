// test phases: ssa, optimized, interpret
// a + b - b == a (was TODO.md: gvn_cancel_operand)
int wain(int a, int b) { return a + b - b; }
