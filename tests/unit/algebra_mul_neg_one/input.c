// test phases: gvn, ssa, optimized, interpret
// a * -1 == 0 - a (was TODO.md: gvn_negate_via_sub). No unary minus in this
// dialect's grammar, so the constant -1 has to arrive via `0 - 1` folding
// (and declaration initializers only accept a literal NUM, hence the
// separate assignment).
int wain(int a, int b) {
  int neg_one = 0;
  neg_one = 0 - 1;
  return a * neg_one;
}
