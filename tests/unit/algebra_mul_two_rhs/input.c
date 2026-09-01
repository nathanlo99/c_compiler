// test phases: gvn, ssa, optimized, interpret
// a * 2 == a + a (was TODO.md: gvn_strength_reduction)
int wain(int a, int b) { return a * 2; }
