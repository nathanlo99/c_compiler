#!/usr/bin/env python3
"""Parallel regression runner: compares live compiler output against the
golden files checked into each tests/<category>/<name>/ directory.
Builds the compiler first by default (pass --skip-build to skip that).

Each case runs harness.DEFAULT_STAGE_NAMES (optimized, mips, interpret) by
default, or the exact subset named by its input.c's own
`// test phases: <name>, <name>, ...` comment if it has one (see
harness.Case.declared_phases) -- e.g. a case that also cares about
register-interference-graph output opts into "compute-rig" this way. Within
a case's stages, evaluated in STAGES order: the moment a stage fails live,
later stages are skipped for that test case, and any golden files that
still exist for those later stages are flagged as "orphan" (stale -- the
test used to get further than it does now, or the goldens were never
cleaned up).

By default, only TODO/FAIL/etc. lines print -- fully-passing (PASS) results
are hidden, to keep a normal run focused on what needs attention. Pass
--verbose to also print PASS. A match against a TO_FIX-marked case prints
as TODO, not PASS -- it's matching today's known-imperfect baseline, not
genuinely fully optimal.

Usage:
    tests/run_tests.py                       # build, run everything, hide PASS
    tests/run_tests.py --verbose              # ...and show PASS too
    tests/run_tests.py --category unit
    tests/run_tests.py --stages optimized,mips
    tests/run_tests.py --pattern loop
    tests/run_tests.py -j 1                   # serial (debugging)
    tests/run_tests.py --require-golden       # missing golden = failure
    tests/run_tests.py --skip-build           # skip the cmake build step
"""
from __future__ import annotations

import argparse
import concurrent.futures
import os
import sys
from collections.abc import Iterator
from dataclasses import dataclass
from typing import Literal

from rich.console import Console
from rich.markup import escape
from rich.progress import (
    BarColumn,
    MofNCompleteColumn,
    Progress,
    TextColumn,
    TimeElapsedColumn,
)

import harness

Status = Literal["match", "todo", "mismatch", "missing", "timeout", "orphan"]


@dataclass
class Outcome:
    """The result of checking one Case's Stage against its golden file."""
    case: harness.Case
    stage: harness.Stage
    status: Status
    result: harness.RunResult | None  # only ever None when status == "orphan"

    @property
    def result_or_raise(self) -> harness.RunResult:
        """`.result`, narrowed to non-None. Every status but "orphan"
        always carries a result; call sites that have already handled (or
        excluded) "orphan" should use this instead of `.result` directly."""
        assert self.result is not None, (
            f"Outcome.result is None for status {self.status!r} (only "
            f"'orphan' outcomes should have no result)"
        )
        return self.result


def evaluate_case(case: harness.Case, requested_stage_names: list[str]) -> list[Outcome]:
    """Runs `case` through STAGES, restricted to the intersection of the
    globally-requested stage_names (e.g. from --stages) and this case's
    own stage_names() (its declared_phases, or DEFAULT_STAGE_NAMES),
    short-circuiting after the first live failure. A golden file for a
    stage the case no longer declares (dropped from its `test phases`
    comment) is just as stale as one orphaned by short-circuiting, and is
    reported the same way -- but only if that stage was itself requested,
    so e.g. `--stages optimized` alone doesn't flag every other stage's
    golden as orphaned."""
    case_stage_names = set(case.stage_names()) & set(requested_stage_names)
    outcomes: list[Outcome] = []
    stopped = False
    for stage in harness.STAGES:
        if stage.name not in case_stage_names:
            if (stage.name in requested_stage_names
                    and harness.golden_paths(case, stage).exists()):
                outcomes.append(Outcome(case, stage, "orphan", None))
            continue
        if stopped:
            if harness.golden_paths(case, stage).exists():
                outcomes.append(Outcome(case, stage, "orphan", None))
            continue

        result = harness.run_stage(case, stage)
        golden_text = harness.read_golden(case, stage)
        if result.timed_out:
            outcomes.append(Outcome(case, stage, "timeout", result))
        elif golden_text is None:
            outcomes.append(Outcome(case, stage, "missing", result))
        elif golden_text == result.golden_text:
            status: Status = "todo" if case.to_fix_reason is not None else "match"
            outcomes.append(Outcome(case, stage, status, result))
        else:
            outcomes.append(Outcome(case, stage, "mismatch", result))

        if result.failed:
            stopped = True
    return outcomes


def compute_outcomes(cases: list[harness.Case], stage_names: list[str],
                     jobs: int) -> Iterator[list[Outcome]]:
    """Evaluates every case in parallel (one worker thread per case, since
    each case's own stages must run sequentially) and yields each case's
    outcomes as soon as that case finishes -- in completion order, not
    sorted, so a caller can print progress live instead of waiting for
    the whole suite."""
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        futures = [pool.submit(evaluate_case, case, stage_names) for case in cases]
        for future in concurrent.futures.as_completed(futures):
            yield future.result()


STYLE = {
    "match": "green",
    "todo": "blue",
    "mismatch": "red",
    "missing": "yellow",
    "timeout": "magenta",
    "orphan": "cyan",
}
SYMBOL = {
    "match": "PASS",
    "todo": "TODO",
    "mismatch": "FAIL",
    "missing": "????",
    "timeout": "TIME",
    "orphan": "ORPH",
}


def format_duration(seconds: float) -> str:
    """Formats a duration for display: milliseconds under 1s, seconds above."""
    if seconds < 1.0:
        return f"{seconds * 1000:.0f}ms"
    return f"{seconds:.1f}s"


def format_outcome(outcome: Outcome) -> str:
    """Builds the Rich-markup PASS/TODO/FAIL/etc. line for `outcome`, with
    its live run's duration -- or nothing, for an orphan (no live run at
    all). A "match" against a TO_FIX-marked case is reported as "todo"
    instead of "match" -- it's not genuinely fully optimal, just matching
    today's known-imperfect baseline -- tagged with the reason regardless
    of status (a TO_FIX-marked case's mismatch, like deep_recursion's, is
    just as much explained by it as its todo/match outcomes are)."""
    label = f"{outcome.case.category}/{outcome.case.name} [{outcome.stage.name}]"
    symbol, style = SYMBOL[outcome.status], STYLE[outcome.status]
    duration = (f" ({format_duration(outcome.result.duration)})"
               if outcome.result is not None else "")
    to_fix = outcome.case.to_fix_reason
    tag = f" [dim]({escape(to_fix)})[/dim]" if to_fix else ""
    return f"[{style}]{symbol:5}[/{style}] {escape(label)}{duration}{tag}"


def tally_text(tally: dict[str, int]) -> str:
    """The live "match=N mismatch=N ..." text shown in the progress bar."""
    return " ".join(f"{k}={v}" for k, v in sorted(tally.items())) or "starting..."


def add_common_args(parser: argparse.ArgumentParser) -> None:
    """Adds the --category/--stages/--pattern filters shared by
    run_tests.py and review_golden.py to `parser`."""
    parser.add_argument("--category", help="comma-separated: " + ",".join(harness.CATEGORIES))
    stage_help = "comma-separated stage names: " + ",".join(harness.STAGES_BY_NAME)
    parser.add_argument("--stages", help=stage_help)
    parser.add_argument("--pattern", help="substring filter on test name")


def resolve_common_args(args: argparse.Namespace) -> tuple[list[harness.Case], list[str]]:
    """Turns the parsed --category/--stages/--pattern filters into the
    matching (cases, stage_names) to evaluate."""
    categories = args.category.split(",") if args.category else None
    stage_names = args.stages.split(",") if args.stages else list(harness.STAGES_BY_NAME)
    cases = harness.discover_cases(categories=categories, pattern=args.pattern)
    return cases, stage_names


def main() -> int:
    """CLI entry point: runs the filtered suite and prints a PASS/FAIL/etc.
    line per stage plus a summary. Returns the process exit code."""
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    add_common_args(parser)
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 4)
    parser.add_argument("--require-golden", action="store_true",
                        help="treat missing golden files as failures")
    parser.add_argument("--verbose", action="store_true",
                        help="also print fully-passing results (PASS); by default "
                             "only TODO/FAIL/etc. print, to focus on what needs attention")
    parser.add_argument("--list", action="store_true", help="list discovered cases and exit")
    parser.add_argument("--no-color", action="store_true")
    parser.add_argument("--skip-build", action="store_true",
                        help="don't run cmake --build first")
    args = parser.parse_args()

    if not args.skip_build and not args.list:
        build_returncode = harness.build_compiler()
        if build_returncode != 0:
            return build_returncode

    cases, stage_names = resolve_common_args(args)
    if args.list:
        for case in cases:
            print(f"{case.category}/{case.name}")
        return 0
    if not cases:
        print("No matching test cases.", file=sys.stderr)
        return 1

    console = Console(no_color=args.no_color)
    tally: dict[str, int] = {}
    with Progress(
        TextColumn("[progress.description]{task.description}"),
        BarColumn(),
        MofNCompleteColumn(),
        TimeElapsedColumn(),
        console=console,
    ) as progress:
        task = progress.add_task(tally_text(tally), total=len(cases))
        for case_outcomes in compute_outcomes(cases, stage_names, args.jobs):
            for outcome in case_outcomes:
                if args.verbose or outcome.status != "match":
                    progress.console.print(format_outcome(outcome))
                tally[outcome.status] = tally.get(outcome.status, 0) + 1
            progress.update(task, advance=1, description=tally_text(tally))

    console.print()
    summary = ", ".join(f"{k}={v}" for k, v in sorted(tally.items())) or "(no stages run)"
    console.print("Summary:", summary)

    failed = tally.get("mismatch", 0) + tally.get("timeout", 0) + tally.get("orphan", 0)
    if args.require_golden:
        failed += tally.get("missing", 0)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
