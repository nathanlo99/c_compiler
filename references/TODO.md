# Optimizer TODO: missing / broken passes

Findings from running `--run-optimizations` over every test in
`tests/cases/optimal/` and `tests/cases/gaps/` and diffing the result
against what the source should reduce to. Each `tests/cases/gaps/<name>/`
directory is the repro for one item below. Run `tests/runner/run_tests.py`
to check against golden outputs, or `tests/runner/review_golden.py` to
accept new/changed ones.

## 1. No reassociation across commutative/associative chains
**Fix:** flatten an add/mul chain and refold its constant operands even
when they're separated by a variable term (currently only two *adjacent*
literals ever get combined).
**Repro:** `tests/cases/gaps/constant_folding/input.c`
**Expected:** `println(a + 1 + b + 1)` → 2 instructions (`add a b` then
`add _ 2`), not 3; `println(0 - a + b)` → 1 instruction (`sub b a`).

## 2. GVN's inverse-cancellation only checks one operand position
**Fix:** in [`global_value_numbering.cpp:104-110`](../src/05_bril_optimization/global_value_numbering.cpp#L104),
the `(a OP b) OP' b -> a` rule only checks `lhs_value.arguments[1]`. Since
commutative operands get canonicalized by an arbitrary complexity/index
tiebreak, the matching operand can land in `arguments[0]` instead — check
both.
**Repro:** `tests/cases/gaps/algebra/input.c`
**Expected:** `a + b - b` → `a` directly (0 instructions), not 2.

## 3. Strength reduction is documented but not implemented
**Fix:** [`global_value_numbering.cpp:122-126`](../src/05_bril_optimization/global_value_numbering.cpp#L122)
comments `// x * 2 == x + x` and `// x * -1 == 0 - x (TODO)` with no code
behind either — add it.
**Repro:** `tests/cases/gaps/algebra/input.c`
**Expected:** `a * 2` → `add a a`, not `mul a 2`.

## 4. No "known branch condition via dominance" elimination
**Fix:** GVN dedupes repeated *values* (e.g. reuses `a < b` for a later
`b > a`) but not repeated *branch outcomes* — once inside the `a < b`-true
arm, a later branch on that same value is statically taken and should
collapse to an unconditional jump. Needs SCCP-style "known facts along this
dominance path" reasoning.
**Repro:** `tests/cases/gaps/gvn/input.c`
**Expected:** collapses all the way to `ret 5` (both arms of the outer `if`
compute the constant 5 once the redundant inner branch is removed —
verified with `--interpret` across inputs).

## 5. No equivalent-arms / jump-threading elimination
**Fix:** `combine_extended_blocks` ([`dead_code_elimination.cpp:171`](../src/05_bril_optimization/dead_code_elimination.cpp#L171))
only merges a block into a sole, single-predecessor successor — it has no
rule for "both arms of this branch do the same trivial thing, so replace
the branch with a jump."
**Repro:** `tests/debug.c` (local scratch file, gitignored — not checked in)
**Expected:** `if (a == b) return 1; else return 1;` → `ret 1`, no branch.

## 6. Local value numbering skips any block touching memory
**Fix:** [`local_value_numbering.cpp:168`](../src/05_bril_optimization/local_value_numbering.cpp#L168)
bails (`if (block.has_loads_or_stores()) return 0;`) on an entire block the
moment it contains *any* load/store, even if most of the block is unrelated
arithmetic. GVN has no equivalent guard but empirically doesn't pick up the
slack. A related, not-fully-traced symptom shows up even with zero memory
ops — loop-carried `id` copies from SSA-destruction also survive when a
same-block substitution should remove them.
**Repro (memory-triggered):** `tests/cases/gaps/vector/input.c`,
`new_and_delete`, `print_all`, `pointer_init`, `print`, `all_tokens`,
`augmented`, `simple2` (all under `tests/cases/gaps/`)
**Repro (loop-only, no memory):** `tests/cases/gaps/simple_loop/input.c`,
`tests/cases/gaps/loop/input.c`
**Expected:** e.g. `%3 = id vec; %4 = id %3; load %4` → `load vec` directly;
loop-carried `%1 = id %0` with no intervening redefinition → uses of `%1`
replaced by `%0` and the copy deleted.

## 7. GVN doesn't dedupe redundant work introduced by inlining
**Fix:** after `min(a,b)` and `max(a,b)` both inline into `pythagoras`, the
identical `lt a b` gets computed twice (once per inlined copy) and never
merged, even though the optimizer reruns after inlining.
**Repro:** `tests/cases/gaps/inline_functions/input.c`
**Expected:** a single `lt a b` shared by both inlined call sites, not two.

## 8. No dead-allocation elimination
**Fix:** [`bril_instruction.hpp:197-201`](../src/04_bril_generation/bril_instruction.hpp#L197)'s
`is_pure()` unconditionally excludes `Alloc`, so an allocation whose result
is never read/freed/escaped can never be DCE'd.
**Repro:** `tests/cases/gaps/memory_leaks/input.c`
**Expected:** `ret 0` — the `alloc` is dead, and once it's gone the loop
around it (whose only effect was that allocation) is dead too.

## 9. Mutual recursion is never inlined (by design — worth revisiting)
**Fix:** [`call_graph_walk.cpp:32-37`](../src/05_bril_optimization/call_graph_walk.cpp#L32)
skips inlining for any function in a multi-member call-graph SCC outright
("inlining makes the program explode: avoid this"). Replace the blanket
skip with a size/depth budget so small, boundable pairs like `isOdd`/`isEven`
don't always pay full call overhead.
**Repro:** `tests/cases/gaps/mutual_recursion/input.c`
**Expected:** at least one level of `isOdd`/`isEven` inlined into each
other under a budget, rather than every call staying a real `call`.

## 10. No tail-call / tail-recursion elimination
**Fix:** nothing in the codebase mentions "tail." Belongs as a BRIL-to-BRIL
pass in `05_bril_optimization`, not a MIPS-codegen special case: rewrite a
self-recursive call in tail position into a backward `jmp` to the
function's own entry block with the loop-carried arguments rebound,
turning the recursion into an ordinary BRIL loop before either downstream
consumer (the MIPS generator, the BRIL interpreter) ever sees it. Doing it
this way — one pass, ahead of both consumers — means the MIPS generator
never has to special-case a self-recursive call at all (it's just emitting
normal branch code for a loop that's already there), and `--interpret`
benefits automatically too: the loop executes at constant native stack
depth like any other loop, instead of recursing natively in C++ once per
BRIL-level call.
**Repro:** `tests/cases/gaps/tail_recursion/input.c`
**Expected:** `odd_part`'s recursive call becomes a backward jump with
updated arguments in the BRIL itself, not a `call` instruction that later
becomes `jalr` + a fresh stack frame — no per-call stack growth for a tail
call, in either the interpreter or the generated MIPS.

Also `tests/cases/gaps/deep_recursion/input.c` (`f(f(f(100000)))`, tail-shaped
recursion via a `result = f(a - 1); return result;` pattern): today's
`--run-optimizations` output has no *other* missed folding/dedup
opportunities in it (nothing left for items 1-9 to catch), but it's not
what the fixed output should look like either -- the BRIL-level pass
described above would rewrite the recursive `call` into a loop entirely,
changing this test's optimized BRIL shape, not just its runtime behavior.
`--interpret`-ing today's (untransformed) BRIL reliably segfaults from
native call-stack overflow at ~100,000 nested frames — a concrete
demonstration of how severe this gap is in practice, not just a
MIPS-stack-growth theoretical one. Confirmed even with the OS stack limit
raised to its hard max (64MB on macOS, 8x the 8MB default) -- still
crashes, and empirically the real threshold is much lower than 100,000 (a
scaled-down copy of this test crashes somewhere between n=5,000 and
n=10,000).

`f(a)` unconditionally reduces to `0` for any `a >= 0` (trivial induction:
base case returns 0, recursive case returns `f(a-1)`), verified against
the real interpreter for n up to 5,000 before it starts crashing -- so
`f(f(f(100000)))` is provably `0` regardless of depth; only the
interpreter's ability to survive that many native stack frames changes
with `n`, not the answer. Its `interpret` golden (`wain returned 0`) is
therefore hand-derived from that proof plus the exact output format
captured from a real completing small-n run, *not* captured from an
actual run of the real (crashing) test -- so it's expected to permanently
show as a `mismatch`, by design, until item 10 is fixed. Once the
BRIL-level pass above exists, this should just start passing along with
`tail_recursion.c` -- no separate interpreter-specific fix needed, since
the interpreter would never see a self-recursive `call` here at all
anymore.

## 11. No loop unrolling / compile-time loop evaluation (stretch goal)
**Fix:** bounded unrolling or symbolic execution for loops whose bounds and
body depend on no unknowns. Lower priority than 1-10 — most production
optimizers don't do this unconditionally either.
**Repro:** `tests/cases/gaps/loop/input.c` — 10 fixed iterations, no
dependency on the function's actual parameters.
**Expected:** `ret 1984` (verified via `--interpret`), no runtime loop at all.

## 12. Register interference graph adds edges for unused arguments
**Fix:** [`liveness_analysis.hpp:51`](../src/05_bril_optimization/data_flow/liveness_analysis.hpp#L51)'s
`RegisterInterferenceGraph` constructor adds an edge between every *pair* of
a function's arguments unconditionally, even ones never read in the body.
Non-`wain` functions get their unused parameters stripped earlier by
`remove_unused_parameters`, but `wain` itself is exempt from that pass, so
this shows up on any test with an unused top-level parameter.
**Repro:** `tests/cases/optimal/simple/input.c` — `wain(int a, int b)`,
`b` unused.
**Expected:** `--compute-rig` output has no edge for `b` (currently:
`b: [a]`, wasting a register-allocation constraint on a dead value).
