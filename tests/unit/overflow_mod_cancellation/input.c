// test phases: ssa, optimized, interpret
// a*3%3 == 0 assumes a*3 doesn't overflow -- signed overflow is UB here
// (references/spec.txt), matching real C. NOT a bug: this cancellation
// fires unconditionally, even for an `a` that overflows at runtime (see
// stdin -- 1.5e9, chosen specifically to overflow `a*3`). Matches `-O2`
// g++ (tests/ground_truth.py); if this ever gets "fixed", it shouldn't be.
int wain(int a, int b) { return a * 3 % 3; }
