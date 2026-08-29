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

Usage:
    tests/review_golden.py                    # interactive, everything
    tests/review_golden.py --category unit
    tests/review_golden.py --accept-all        # non-interactive bootstrap
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


def review(outcomes: list[Outcome], auto_accept: bool) -> tuple[int, int]:
    """Walks every pending (missing/mismatch/orphan) outcome, prompting for
    each (or auto-accepting all of them if `auto_accept`). Returns
    (accepted, skipped) counts."""
    accepted = skipped = 0
    pending = [o for o in outcomes if o.status in ("missing", "mismatch", "orphan")]
    total = len(pending)
    for idx, outcome in enumerate(pending, 1):
        case, stage = outcome.case, outcome.stage
        header = f"[{idx}/{total}] {case.category}/{case.name} [{stage.name}]  ({outcome.status})"
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
                return accepted, skipped
            print("unrecognized choice")
    return accepted, skipped


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
                        help="non-interactive: accept every pending change as golden")
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

    accepted, skipped = review(outcomes, args.accept_all)
    print(f"\nDone. Accepted {accepted}, skipped {skipped}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
