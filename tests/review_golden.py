#!/usr/bin/env python3
"""Interactively review and accept new/changed compiler outputs as golden
files under each tests/<category>/<name>/ directory. Builds the
compiler first by default (pass --skip-build to skip that).

For every (test case, stage) pair whose live output is missing, different
from the checked-in golden, or "orphaned" (a golden exists for a stage that
a now-earlier failure makes unreachable), shows a diff and prompts:

    [y] accept the new output as golden (or, for an orphan, delete it)
    [n] skip (leave as-is)
    [d] show the diff again
    [s] show the full new output (not just the diff)
    [q] quit (everything accepted so far is already saved)

--accept-all holds back TO_FIX-marked cases by default -- "is this new
output actually correct now, or just differently imperfect" is exactly the
call those need a human for, and this is the one way this tool can corrupt
a hand-verified golden instead of just being slow to update it. Pass
--include-to-fix to sweep those in too.

Usage:
    tests/review_golden.py                    # interactive, everything
    tests/review_golden.py --category unit
    tests/review_golden.py --accept-all        # non-interactive, holds back TO_FIX cases
    tests/review_golden.py --accept-all --include-to-fix  # ...and those too
    tests/review_golden.py --skip-build        # skip the cmake build step
"""
from __future__ import annotations

import argparse
import difflib
import sys

from rich.progress import track

import harness
from run_tests import Outcome, add_common_args, compute_outcomes, resolve_common_args


def print_diff(outcome: Outcome) -> None:
    """Prints a unified diff of `outcome`'s live output against its current
    golden (or a note that it's orphaned)."""
    if outcome.status == "orphan":
        print("(orphaned golden -- an earlier stage now fails, so this "
              "stage is no longer reached)")
        return
    result = outcome.result_or_raise
    golden_text = harness.read_golden(outcome.case, outcome.stage)
    old_lines = golden_text.splitlines(keepends=True) if golden_text is not None else []
    new_lines = result.golden_text.splitlines(keepends=True)
    diff = list(difflib.unified_diff(
        old_lines, new_lines,
        fromfile="golden" if golden_text is not None else "(missing)",
        tofile="live",
    ))
    if diff:
        print("".join(diff), end="" if diff[-1].endswith("\n") else "\n")
    else:
        print("(no output)")


def _split_pending(outcomes: list[Outcome], auto_accept: bool,
                   include_to_fix: bool) -> tuple[list[Outcome], list[Outcome]]:
    """Splits `outcomes` into (pending, held_back): held_back is TO_FIX-marked
    pending outcomes when auto_accept and not include_to_fix, else empty."""
    pending = [o for o in outcomes if o.status in ("missing", "mismatch", "orphan")]
    if not (auto_accept and not include_to_fix):
        return pending, []
    return (
        [o for o in pending if o.case.to_fix_reason is None],
        [o for o in pending if o.case.to_fix_reason is not None],
    )


def review(outcomes: list[Outcome], auto_accept: bool,
          include_to_fix: bool = False) -> tuple[int, int, int]:
    """Walks every pending (missing/mismatch/orphan) outcome, prompting for
    each (or auto-accepting all of them if `auto_accept`). Returns
    (accepted, skipped, held_back) counts.

    If `auto_accept` and not `include_to_fix`, TO_FIX-marked cases are held
    back entirely -- not accepted, not prompted, not touched -- since
    "is this new output actually correct now, or just differently
    imperfect" is exactly the judgment call a TO_FIX case needs a human
    for, and --accept-all sweeping over one of those silently is the one
    way this tool can actively corrupt a hand-verified golden instead of
    just being slow to update it. Pass --include-to-fix to opt back in."""
    accepted = skipped = 0
    pending, held_back = _split_pending(outcomes, auto_accept, include_to_fix)
    total = len(pending)
    for idx, outcome in enumerate(pending, 1):
        case = outcome.case
        header = (f"[{idx}/{total}] {case.category}/{case.name} "
                 f"[{outcome.stage.name}]  ({outcome.status})")
        print("=" * len(header))
        print(header)
        print("=" * len(header))
        print(f"source: {case.c_path.relative_to(harness.REPO_ROOT)}")
        print_diff(outcome)

        if auto_accept:
            _apply(outcome)
            accepted += 1
            print("-> accepted (auto)\n")
            continue

        while True:
            prompt = ("[y]delete / [n]skip / [q]uit > " if outcome.status == "orphan"
                     else "[y]accept / [n]skip / [d]iff / [s]how full / [q]uit > ")
            try:
                choice = input(prompt).strip().lower()
            except EOFError:
                choice = "q"
            if choice in ("y", "yes"):
                _apply(outcome)
                accepted += 1
                print("-> accepted\n")
                break
            if choice in ("n", "no", ""):
                skipped += 1
                print("-> skipped\n")
                break
            if choice == "d" and outcome.status != "orphan":
                print_diff(outcome)
                continue
            if choice == "s" and outcome.status != "orphan":
                print(outcome.result_or_raise.golden_text)
                continue
            if choice == "q":
                print(f"\nQuitting. Accepted {accepted}, skipped {skipped}, "
                      f"{total - idx} not reviewed.")
                return accepted, skipped, len(held_back)
            print("unrecognized choice")

    if held_back:
        names = ", ".join(f"{o.case.category}/{o.case.name} [{o.stage.name}]"
                          for o in held_back)
        print(f"Held back {len(held_back)} TO_FIX-marked outcome(s) -- rerun "
              f"without --accept-all (or with --include-to-fix) to review "
              f"them: {names}")
    return accepted, skipped, len(held_back)


def _apply(outcome: Outcome) -> None:
    if outcome.status == "orphan":
        harness.clear_golden(outcome.case, outcome.stage)
    else:
        harness.write_golden(outcome.case, outcome.stage, outcome.result_or_raise)


def main() -> int:
    """CLI entry point: computes outcomes for the filtered suite, then
    reviews (interactively or via --accept-all) whatever's pending."""
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    add_common_args(parser)
    parser.add_argument("--accept-all", action="store_true",
                        help="non-interactive: accept every pending change as golden "
                             "(holds back TO_FIX-marked cases -- see --include-to-fix)")
    parser.add_argument("--include-to-fix", action="store_true",
                        help="with --accept-all, also auto-accept TO_FIX-marked cases "
                             "instead of holding them back for manual review")
    parser.add_argument("--skip-build", action="store_true",
                        help="don't run cmake --build first")
    args = parser.parse_args()

    if not args.skip_build:
        build_returncode = harness.build_compiler()
        if build_returncode != 0:
            return build_returncode

    cases, stage_names = resolve_common_args(args)
    progress = track(compute_outcomes(cases, stage_names, jobs=8),
                     total=len(cases), description="Evaluating cases...")
    outcomes = [o for case_outcomes in progress for o in case_outcomes]
    outcomes.sort(key=lambda o: (o.case.category, o.case.name, o.stage.name))

    pending = [o for o in outcomes if o.status in ("missing", "mismatch", "orphan")]
    if not pending:
        print("Nothing to review -- everything already matches golden.")
        return 0

    counts = {s: sum(1 for o in pending if o.status == s)
             for s in ("missing", "mismatch", "orphan")}
    print(f"{len(pending)} case(s) need review "
          f"(missing={counts['missing']}, mismatch={counts['mismatch']}, "
          f"orphan={counts['orphan']}).\n")

    accepted, skipped, held_back = review(outcomes, args.accept_all, args.include_to_fix)
    held_back_note = f", held back {held_back}" if held_back else ""
    print(f"\nDone. Accepted {accepted}, skipped {skipped}{held_back_note}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
