// test phases: optimized, interpret
// a + b - b == a (TODO.md item 2: GVN inverse-cancellation operand-order bug)
int wain(int a, int b) { return a + b - b; }
