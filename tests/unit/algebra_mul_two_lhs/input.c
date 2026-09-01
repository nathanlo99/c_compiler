// test phases: gvn, ssa, optimized, interpret
// 2 * a == a + a (was TODO.md: gvn_strength_reduction)
int wain(int a, int b) { return 2 * a; }
