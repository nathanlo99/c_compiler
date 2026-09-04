# A general sparse conditional propagation framework

Design doc for the `sccp` item in `references/TODO.md`. Constant
propagation is the motivating instantiation, but the sparse/conditional
worklist machinery itself doesn't know or care that the lattice is about
constants -- it's parameterized over a `Lattice` and a `Transfer` policy,
so a second instantiation (copy propagation) and a third (value ranges,
which also gets comparisons resolving to constants during the analysis
for free) are each a new pair of small types, not a new engine. A fourth
piece, `assume`, extends this to dominance-based facts a per-SSA-name
lattice can't express on its own -- `if (a) { if (!a) ... }` folding the
inner branch away.

## Why generalize instead of just writing SCCP directly

Two reasons, not just "abstraction for its own sake":

1. `ssa_copy_propagation.cpp` already exists and does its own worklist-free
   substitution pass for exactly one narrow lattice (`x` is a copy of `y`,
   or it isn't). If SCCP ships as a hardcoded constant-only engine, that
   pass either stays a permanent duplicate or `sccp` has to special-case
   copies internally. Generalizing means copy propagation becomes a
   *second instantiation of the same engine*, and the duplication actually
   goes away.
2. This is the same shape `persistent_value_graph` and `worklist_fixpoint`
   are gesturing at -- dirty-tracking over a real def-use graph instead of
   rescanning. Building that machinery once, generically, is a smaller
   step than it looks once SCCP needs it anyway.

## The two moving parts

**`Lattice`** -- the abstract domain of "what do we know about this SSA
value." Needs a top (nothing known yet, optimistic starting point), a
bottom (proven to vary at runtime, give up), and a `meet` that combines
two facts about the same value into the most precise fact still true of
both. `meet` must be commutative, associative, and idempotent -- that's a
mathematical requirement on any implementation, not something the type
system checks.

```cpp
template <typename L>
concept Lattice = std::equality_comparable<L> && requires(const L &a, const L &b) {
  { L::top() } -> std::same_as<L>;
  { L::bottom() } -> std::same_as<L>;
  { meet(a, b) } -> std::same_as<L>;  // free function, ADL-found -- matches this
                                      // codebase's operator<< convention
};
```

**`Transfer`** -- opcode-specific knowledge: given an instruction and the
current lattice values of its operands, what's the lattice value of its
result? And, for a conditional branch specifically, which successor
label(s) does that value make executable? Everything opcode-aware lives
here; the engine never switches on `Opcode` itself except to special-case
`Phi` (generic, needs no `Transfer` involvement -- see below) and
unconditional `Jmp`/`Ret` (trivially always-executable, no lattice
question to ask).

```cpp
template <typename T, typename L>
concept Transfer = Lattice<L> && requires(const T &t, const Instruction &instr,
                                          const std::vector<L> &operands) {
  { t.evaluate(instr, operands) } -> std::same_as<L>;
  { t.branch_targets(instr, operands) } -> std::same_as<std::vector<std::string>>;
};
```

A `Transfer` that doesn't care about reachability refinement at all can
just implement `branch_targets` to conservatively return both successors
always -- that degenerates to plain (non-conditional) sparse propagation
as a special case, for free.

## The engine

```cpp
template <typename L, typename T>
  requires Lattice<L> && Transfer<T, L>
class SparseConditionalPropagation {
  ControlFlowGraph &function;
  T transfer;

  std::unordered_map<std::string, L> lattice;               // starts at top();
                                                              // parameters seeded bottom()
  std::set<std::pair<std::string, std::string>> executable_edges;  // (from, to)
  std::unordered_set<std::string> reachable_blocks;

  // var -> (block, instruction index): built once, SSA form guarantees
  // exactly one def per name so this doesn't need to be persistent
  // (see "Relationship to persistent_value_graph" below)
  std::unordered_map<std::string, std::pair<std::string, size_t>> def;
  std::unordered_map<std::string, std::vector<std::pair<std::string, size_t>>> uses;

  std::deque<std::pair<std::string, std::string>> cfg_worklist;
  std::deque<std::string> ssa_worklist;

public:
  void run();               // the fixpoint loop
  size_t apply();            // rewrite provably-constant defs and
                              // constant-condition branches; returns count changed
};
```

`lattice.at(name)` is the value a use should read; anything not yet in the
map defaults to `top()`. Building `def`/`uses` is one linear pass over the
function and doesn't touch the `Lattice`/`Transfer` types at all -- fully
generic.

### The fixpoint loop

Two worklists, seeded with the entry block's outgoing edge(s) and nothing
else -- everything starts pessimistically unreachable and optimistically
`top()`, which is what lets the algorithm discover a loop-carried value is
constant when a value that only ever moves *down* from `bottom()` never
could.

```
while cfg_worklist or ssa_worklist not empty:
  if cfg_worklist not empty:
    (from, to) = pop cfg_worklist
    if (from, to) in executable_edges: continue
    mark (from, to) executable
    first_time = to not in reachable_blocks
    mark to in reachable_blocks
    for each Phi in to: visit_phi(phi)          # always -- a new edge can
                                                  # change what an existing
                                                  # phi merges
    if first_time:
      for each non-phi instruction in to, in order: visit(instruction)

  elif ssa_worklist not empty:
    v = pop ssa_worklist
    for (block, idx) in uses[v]:
      if block not in reachable_blocks: continue  # don't waste work on dead code
      visit(instructions[block][idx])
```

`visit_phi`: `new_value = meet over { lattice[arg] : (label, arg) in
phi's incoming pairs, (label, phi.block) in executable_edges }`, starting
from `top()` (so zero executable incoming edges naturally gives `top()`).
If it changed, update `lattice[phi.dest]` and push `phi.dest` onto
`ssa_worklist`.

`visit(instruction)` for anything else:
- `Jmp`: push its one successor edge onto `cfg_worklist` unconditionally.
- `Br`: call `transfer.branch_targets(instr, operand_values)`, push each
  named successor's edge. A `Transfer` that's proven the condition
  constant returns just the taken label; unresolved or proven-varying
  returns both.
- Anything else with a destination: `new_value = transfer.evaluate(instr,
  operand_values)`. If changed, update `lattice[dest]`, push `dest` onto
  `ssa_worklist`.

### Apply phase

Deliberately thin, and deliberately not where cleanup happens:

- Every SSA value with a `Constant`-shaped lattice result (the constant
  instantiation exposes this via its own accessor, since the engine
  doesn't know what "constant" means for a general `L`) gets its defining
  instruction rewritten to a literal `const`.
- Every reachable `Br` whose condition resolved to that same
  constant-shaped result gets rewritten to an unconditional `Jmp` to the
  taken label.
- Nothing else. No block deletion, no dead-phi-argument pruning, no DCE.
  `remove_unused_blocks` and the existing DCE passes, already in
  `run_optimization_passes`'s fixpoint, pick up everything this severs --
  reusing tested cleanup machinery instead of re-implementing it inside
  a new pass that has no business owning that responsibility.

## Instantiation 1: constant propagation (`sccp`)

```cpp
struct ConstLattice {
  enum class Kind { Top, Constant, Bottom } kind = Kind::Top;
  int value = 0;  // valid iff kind == Constant

  static ConstLattice top() { return {}; }
  static ConstLattice bottom() { return {Kind::Bottom, 0}; }
  bool operator==(const ConstLattice &) const = default;
};

ConstLattice meet(const ConstLattice &a, const ConstLattice &b) {
  if (a.kind == ConstLattice::Kind::Top) return b;
  if (b.kind == ConstLattice::Kind::Top) return a;
  if (a.kind == ConstLattice::Kind::Bottom || b.kind == ConstLattice::Kind::Bottom)
    return ConstLattice::bottom();
  return a.value == b.value ? a : ConstLattice::bottom();
}

struct ConstTransfer {
  ConstLattice evaluate(const Instruction &instr, const std::vector<ConstLattice> &ops) const;
  std::vector<std::string> branch_targets(const Instruction &instr,
                                          const std::vector<ConstLattice> &ops) const;
};
```

`ConstTransfer::evaluate` reuses the exact same `foldable_ops`-style
arithmetic tables `simplify_binary` already has (unsigned-cast wraparound,
same `references/spec.txt` overflow-is-UB stance -- no new semantics to
invent here, just a new place the same rules get applied from). Any
operand still at `top()` keeps the result `top()` (optimistic: haven't
proven it varies yet); any operand at `bottom()` with no fold rule that
survives it makes the result `bottom()`.

`ConstTransfer::branch_targets`: if the condition operand is `Constant`,
return the one taken label; otherwise (top or bottom) return both.

This instantiation is what obsoletes GVN/LVN's constant-folding and
phi-collapse-to-`id` logic -- see `references/TODO.md`'s `sccp` entry for
the full accounting. Their CSE/value-numbering role (recognizing two
*non-constant* expressions compute the same value) is untouched; nothing
here does that. It only obsoletes the easy slice of `gvn_dominance_branch`
(a value that's globally constant staying propagated to every use) --
the genuinely path-sensitive case needs the predicate-info extension
below, since plain SSA gives `a` the same name inside and outside a
branch dominated by a fact about it.

## Instantiation 2: copy propagation

Sketched to prove the engine is actually generic, not just constant-shaped
with extra steps:

```cpp
struct CopyLattice {
  enum class Kind { Top, SameAs, Bottom } kind = Kind::Top;
  std::string root;  // valid iff kind == SameAs; always fully resolved,
                      // never another SameAs -- see meet() below

  static CopyLattice top() { return {}; }
  static CopyLattice bottom() { return {Kind::Bottom, ""}; }
  bool operator==(const CopyLattice &) const = default;
};

CopyLattice meet(const CopyLattice &a, const CopyLattice &b) {
  if (a.kind == CopyLattice::Kind::Top) return b;
  if (b.kind == CopyLattice::Kind::Top) return a;
  if (a.kind == CopyLattice::Kind::Bottom || b.kind == CopyLattice::Kind::Bottom)
    return CopyLattice::bottom();
  return a.root == b.root ? a : CopyLattice::bottom();
}
```

`CopyTransfer::evaluate` only has one interesting case: `x = id y` gives
`lattice[y]` directly if `y` already resolved to `SameAs`/`Bottom`,
otherwise `SameAs{y}`. Every other opcode is unconditionally `bottom()` --
this lattice has nothing to say about arithmetic, only about aliasing.
`branch_targets` conservatively returns both successors always (copies
don't make branches decidable). Apply phase substitutes every use of `x`
with `root` wherever `lattice[x]` is `SameAs`. This *is*
`ssa_copy_propagation`, rebuilt on the shared engine instead of its own
bespoke pass -- once this instantiation exists, the standalone pass can go.

(A combined constant-or-copy lattice, `Top | Constant(int) | SameAs(name)
| Bottom`, is also possible and would subsume both instantiations into
one pass -- mentioned in `TODO.md`'s `sccp` entry as the "needs a
deliberate lattice extension, not free" case. Left as two instantiations
here for clarity; merging them is a later decision, not a prerequisite.)

## Instantiation 3: value ranges

Same shape again, ordered top-to-bottom by "how much do we know" exactly
like `ConstLattice`, just with a continuous middle instead of a flat one:

```cpp
struct RangeLattice {
  enum class Kind { Top, Range, Bottom } kind = Kind::Top;
  int lo = 0, hi = 0;  // valid iff kind == Range. lo == hi is a specific
                        // integer -- not a fourth kind, just the narrowest
                        // Range, sitting right where Constant sits above

  static RangeLattice top() { return {}; }
  static RangeLattice bottom() { return {Kind::Bottom, 0, 0}; }
  bool operator==(const RangeLattice &) const = default;
};

RangeLattice meet(const RangeLattice &a, const RangeLattice &b) {
  if (a.kind == RangeLattice::Kind::Top) return b;
  if (b.kind == RangeLattice::Kind::Top) return a;
  if (a.kind == RangeLattice::Kind::Bottom || b.kind == RangeLattice::Kind::Bottom)
    return RangeLattice::bottom();
  // widening to the smallest covering range moves toward less precision,
  // same direction as ConstLattice's meet, just continuous instead of flat
  return RangeLattice{RangeLattice::Kind::Range, std::min(a.lo, b.lo), std::max(a.hi, b.hi)};
}
```

A real implementation would want a fourth case (or a distinguished
`Range` with `lo > hi`) for "unreachable," and a widening operator so
merging genuinely-disjoint ranges at a loop back-edge doesn't collapse
straight to `bottom()` after one iteration -- both real design decisions
for whenever this gets built, not sketched further here.

### Comparisons resolve to constants *during* the analysis, not after

A comparison's result is representable in this same lattice: a boolean is
just `Range{0,0}` (false) or `Range{1,1}` (true) -- a degenerate
single-point range, nothing new. So `ConstTransfer`/`RangeTransfer`'s
`evaluate()` for `Lt`/`Le`/`Gt`/`Ge`/`Eq`/`Ne` can decide the comparison is
a known boolean even when *neither* operand is a single point, as long as
their ranges don't overlap in a way that leaves the outcome ambiguous:

```cpp
RangeLattice evaluate_lt(const RangeLattice &lhs, const RangeLattice &rhs) {
  if (lhs.kind == RangeLattice::Kind::Top || rhs.kind == RangeLattice::Kind::Top)
    return RangeLattice::top();          // don't know yet, wait and see
  if (lhs.kind == RangeLattice::Kind::Bottom || rhs.kind == RangeLattice::Kind::Bottom)
    return RangeLattice{RangeLattice::Kind::Range, 0, 1};  // could be either --
                                          // still narrower than bottom(): the
                                          // *result* of a comparison is always
                                          // in {0,1} even when an operand isn't
                                          // bounded at all
  if (lhs.hi < rhs.lo) return RangeLattice{RangeLattice::Kind::Range, 1, 1};  // true
  if (lhs.lo >= rhs.hi) return RangeLattice{RangeLattice::Kind::Range, 0, 0}; // false
  return RangeLattice{RangeLattice::Kind::Range, 0, 1};                      // genuinely either
}
```

`Range[3, 5] < 6` hits `lhs.hi=5 < rhs.lo=6` (6 modeled as `Range{6,6}`) →
`Range{1,1}`. That result then flows into a `Br`'s condition through the
exact same `ssa_worklist`/`branch_targets` path any other resolved value
does -- `branch_targets` just checks "is this operand a single-point
range" and, if so, returns only the taken label. No separate pass reads
the finished analysis and re-derives this afterward; it falls out of the
same fixpoint a plain constant does, just because the *lattice* -- not the
engine -- knows how to compare ranges instead of only equality-checking
two exact values.

This is also why it's worth building on ranges rather than stopping at
constants: `Range[3,5] < 6` is a real, common pattern (loop bounds,
array-index guards) a constant-only lattice can never resolve, since
neither operand is ever a single exact value. It's the same idea as LLVM's
`CorrelatedValuePropagation`/GCC's VRP -- range-aware conditional constant
propagation is a well-trodden, not speculative, direction.

## Extension: `assume` -- a general fact primitive

The motivating case: `if (a) { ... if (!a) { /* provably dead */ } }`.
Nothing above handles this -- SSA renaming only happens at assignments,
never at branches, so `a` inside the region dominated by `if (a)`'s true
edge is the *same SSA name* as `a` everywhere else in the function. The
lattice is keyed per-SSA-name, one fact for the whole function; it has no
way to say "normally unconstrained, but specifically inside this
dominated region, known nonzero." This is a real, separate capability,
not a corollary of range-SCCP -- worth being precise about, since an
earlier draft of this doc overclaimed that plain `sccp` subsumes
`gvn_dominance_branch` outright. It only subsumes the easy slice (a value
that's *globally* constant staying propagated to every use, which is
ordinary SCCP).

Rather than a side-table keyed by destination name and populated only by
branch analysis, make it a real instruction any pass can emit and the
engine consumes uniformly -- predicate info becomes the *first* producer,
not a special case baked into the engine. Two more things fall out of
building it this way, both real wins, not just nice framing: the
interpreter can verify every fact any pass ever asserts, for free, on
every test that already gets interpreted; and other passes (an
allocation-bounds pass proving `idx < len` at an array access, say) get
the same mechanism without inventing their own.

**Shape**: `assume lhs <comparison> rhs;` -- six opcodes, `AssumeLt`
through `AssumeNe`, `arguments = {lhs, rhs}`, no destination. One opcode
per comparison rather than one opcode plus a comparison field, same split
this project already uses for `Lt`/`Le`/`Gt`/`Ge`/`Eq`/`Ne` themselves.
Structured rather than a single boolean SSA value, so a consumer reads
the comparison directly instead of tracing back through a defining
instruction -- and both sides are names, never literals, since BRIL has
no literal operands (`assume a lt %100;`, `%100` the name holding 100).
`idx < len` (both variable) is exactly this too, no special casing.
Living in the instruction stream, not attached to a block or another
instruction, is the whole point: it can sit anywhere, survives
block/instruction mutation elsewhere for free (nothing points at it, it
doesn't point at anything by position), and deleting or moving it is
exactly as safe as deleting or moving any other instruction.

**The engine's use of it**: `SparseConditionalPropagation` already walks
each block's instructions forward in order; each `AssumeX` is just
another case in that per-instruction dispatch, reached exactly when
control would reach it, turned directly into a `RangeLattice` narrowing
with no lookup needed:

```cpp
RangeLattice RangeTransfer::refine(const RangeLattice &current,
                                   const Instruction &assume) const {
  const auto bound = lattice.find(assume.arguments[1]);
  if (bound.kind != RangeLattice::Kind::Range || bound.lo != bound.hi)
    return current;                                 // rhs not resolved to a point yet
  const int b = bound.lo;
  switch (assume.opcode) {
    case Opcode::AssumeLt: return narrow(current, {RangeLattice::Kind::Range, INT_MIN, b - 1});
    case Opcode::AssumeLe: return narrow(current, {RangeLattice::Kind::Range, INT_MIN, b});
    case Opcode::AssumeGt: return narrow(current, {RangeLattice::Kind::Range, b + 1, INT_MAX});
    case Opcode::AssumeGe: return narrow(current, {RangeLattice::Kind::Range, b, INT_MAX});
    case Opcode::AssumeEq: return {RangeLattice::Kind::Range, b, b};
    case Opcode::AssumeNe: return current;          // a hole, not an interval
    default: return current;
  }
}
```

`narrow` here means "intersect," not the lattice's own `meet` -- combining
a pre-existing fact with a newly discovered one should only ever get
*more* precise, the opposite direction plain `meet` moves. That's a real
addition beyond the `Lattice` concept as sketched earlier in this doc.

**Populating it -- predicate info as the first producer**: for `br %c
L_true L_false` where `%c = <cmp> X Y`, materialize `%c`'s own value
directly as a `const` in each successor (unchanged from before) *and*
emit the (negated, on the false edge) comparison as an `assume`:

```
br %c L_true L_false     // %c = lt a %100
L_true:
  %c.1 = const 1          // %c's own refined value in this branch
  assume a lt %100;        // redundant with the const fold above for this
                            // specific case, but uniform -- costs nothing
L_false:
  assume a ge %100;        // negated -- Lt<->Ge, Le<->Gt, Eq<->Ne
```

Nested `if (a) { if (!a) ... }` is exactly this: the inner branch's own
condition resolves through `%c.1`/`%c.2` being `const`s, no `assume`
needed for that part specifically -- `assume` earns its keep on cases the
`const`-materialization half can't reach, like the range-refinement
example above.

**The interpreter checks these at runtime**: evaluates `lhs comparison
rhs` and throws if false -- the same `debug_assert`-style failure this
project's debug/ASan build already uses to catch real bugs (the
`query_or_insert` bug this session was found exactly this way). Every
fact any pass ever asserts gets validated on every test that already has
an `interpret` phase, for free. Costs nothing in the actual compiled MIPS
output: `assume` never lowers to any instruction there.

## Relationship to `persistent_value_graph`

The `def`/`uses` maps above are real def-use edges, but built fresh per
engine run and scoped to one function -- not persistent across passes.
That's sufficient here because SCCP only ever runs on SSA-form code (same
precondition GVN already asserts), and SSA guarantees each name has
exactly one definition, so there's no "which definition does this name
mean right now" ambiguity to resolve. `persistent_value_graph` is the
generalization of this same idea to *every* pass, kept live across the
whole pipeline instead of rebuilt per-run -- worth doing eventually, but
not a blocker for building this.

## Where this lives

- `src/05_bril_optimization/sparse_conditional_propagation.hpp` -- the
  generic engine: `Lattice`/`Transfer` concepts, the
  `SparseConditionalPropagation<L, T>` class, the def-use index builder.
  No knowledge of constants or copies.
- `src/05_bril_optimization/sccp.hpp`/`.cpp` -- `ConstLattice`,
  `ConstTransfer`, and the `size_t sccp(ControlFlowGraph &function)` free
  function `run_optimization_passes` actually calls (same calling
  convention as `global_value_numbering`).
- `ssa_copy_propagation.hpp`/`.cpp` stays as-is until instantiation 2
  above is built and proven equivalent on the existing test suite: then
  it gets replaced by a thin `CopyLattice`/`CopyTransfer` pair and the
  original file deleted, not kept alongside.
- `AssumeLt`..`AssumeNe` -- landed in `bril_instruction.hpp`/`.cpp` and
  `bril_interpreter.cpp` already (see `sccp_assume`); nothing emits them
  yet. Populating them (predicate info) is its own small pass, not part
  of the engine itself.

## Open questions to resolve while implementing, not before

- Does the engine need to run on pointer-touching functions at all, or
  should `sccp` mirror GVN's `uses_pointers()` bailout for a first cut?
  Nothing about the constant lattice specifically requires bailing --
  `Load`/`Store` arguments are ordinary SSA names like any other -- but a
  narrower first version is a reasonable place to start.
- Exact placement in `run_optimization_passes`'s loop: right after GVN,
  same reasoning as `ssa_copy_propagation`'s placement -- catch what GVN's
  own round just created the same round.
- Whether `apply()` should live on the engine itself (as sketched) or be
  pulled out per-instantiation, if `ConstLattice`'s and `CopyLattice`'s
  apply logic end up sharing less than expected in practice.
