# Test runner

Golden-file regression testing over `tests/`. Replaces the old
turnt-based `tests/turnt/` setup (removed) — one uniform mechanism now,
instead of a build_program/run_program split.

## Setup

```bash
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt        # click, rich -- to run the scripts
.venv/bin/pip install -r requirements-dev.txt     # + ruff, mypy, pylint -- to lint them
```

`.venv/` is gitignored; recreate it locally, don't commit it. Run the tools
via `.venv/bin/python3 tests/run_tests.py ...` (or activate the venv
first with `source .venv/bin/activate` and drop the `.venv/bin/` prefix).

## Layout

```
tests/<category>/<name>/
    input.c        the C program under test
    stdin           (optional) stdin for the "interpret" stage;
                    defaults to "5 10\n" if absent
    <stage>.out     golden output for that pipeline stage
```

`<category>` is one of `unit`, `benchmarks`, `broken`. `<stage>` is one of
`optimized`, `mips`, `interpret` (see `harness.py`'s `STAGES`), run in that
order. `ast`/`bril` (the pre-optimization stages) aren't tracked here —
this suite is about the optimizer/codegen work `references/TODO.md`
tracks, not the frontend.

**A `unit/<name>/TO_FIX` file** means that test is a known, deliberate
repro for one `references/TODO.md` item (the file names which) — its
`optimized`/`mips` golden captures today's *known-imperfect* output as the
regression baseline, not the fully-optimal form `TODO.md` describes as
"Expected". No `TO_FIX` file means the test is expected to already be
fully optimal; a `mismatch` there is a real regression. (There used to be
a directory-level `optimal`/`gaps` split doing this job instead — dropped
because it conflated "what kind of test is this" with "is it currently
optimal," didn't apply to `benchmarks/` at all, and needed a `git mv`
every time a test's status changed.)

Return codes aren't golden-checked (see the comment atop `harness.py` for
why) — only stdout/stderr text is. The live return code still matters at
runtime, though: it's what drives short-circuiting below.

**Short-circuiting:** the moment a stage's live run returns non-zero, later
stages are skipped for that test case entirely — not run, not golden-checked.
A test that's meant to fail to parse (`tests/broken/*`) has only an
`optimized.out` (its first and only reachable stage); there's no `mips.out`
etc., and there shouldn't be — absence means "doesn't get this far," not
"produces nothing." If a code change makes an earlier stage start failing,
any golden files for now-unreachable later stages are reported as
**orphaned** by both tools below, so they get cleaned up rather than
silently going stale.

## Running the suite

```bash
tests/run_tests.py                    # everything
tests/run_tests.py --category unit
tests/run_tests.py --stages optimized,mips
tests/run_tests.py --pattern loop
tests/run_tests.py -j 1                # serial, e.g. for debugging
tests/run_tests.py --require-golden    # missing golden counts as failure (CI mode)
```

Runs are parallelized across test cases (`-j`, defaults to CPU count); each
individual compiler invocation has a 15s timeout
(`harness.DEFAULT_TIMEOUT`) — `benchmarks/lexer`'s `optimized` stage is a
known slow case that can exceed this on some machines.

Each stage's PASS/FAIL/etc. line prints live as it completes (not sorted —
completion order) with that stage's live-run duration (`PASS unit/simple
[optimized] (4ms)`), and a persistent progress bar underneath shows the
running tally (`match=N mismatch=N ...`) and cases remaining.

Exit code is non-zero if any `mismatch`/`timeout`/`orphan` was found
(add `--require-golden` to also fail on `missing`).

## Reviewing / accepting golden output

```bash
tests/review_golden.py                 # interactive, everything pending
tests/review_golden.py --category unit --pattern loop
tests/review_golden.py --accept-all     # non-interactive bootstrap
```

For each pending case it shows a diff against the current golden (or the
full new output, if there wasn't one yet) and prompts:

- `y` — accept the live output as the new golden (or, for an orphan, delete
  the stale golden files)
- `n` — skip, leave as-is
- `d` — re-show the diff
- `s` — show the full new output, not just the diff
- `q` — quit; everything accepted so far is already written to disk

`--accept-all` skips the prompts and accepts everything — useful for bulk
bootstrapping a category you've already verified by hand (e.g. `broken/`,
where "the current error message" is unambiguously the right golden), but
not a substitute for actually reviewing `TO_FIX`-marked tests' outputs,
since those are *expected* to be imperfect (see `references/TODO.md`).
