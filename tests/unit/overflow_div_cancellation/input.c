// test phases: optimized, interpret
// a*2/2 == a assumes a*2 doesn't overflow -- signed overflow is UB here
// (references/spec.txt), matching real C. NOT a bug: this cancellation
// fires unconditionally, even for an `a` that overflows at runtime (see
// stdin -- 2^30, chosen specifically to overflow `a*2`). Matches `-O2`
// g++ (tests/ground_truth.py); if this ever gets "fixed", it shouldn't be.
int wain(int a, int b) { return a * 2 / 2; }
