// test phases: ssa, optimized, interpret
// 0 - a + b == b - a (TODO.md item 1: reassociation not implemented)
int wain(int a, int b) { return 0 - a + b; }
