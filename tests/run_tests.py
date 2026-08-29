#!/usr/bin/env python3
"""Parallel regression runner: compares live compiler output against the
golden files checked into each tests/<category>/<name>/ directory.
Builds the compiler first by default (pass --skip-build to skip that).

Stages run in pipeline order (optimized, mips, interpret); the moment a
stage fails live, later stages are skipped for that test case, and any
golden files that still exist for those later stages are flagged as
"orphan" (stale -- the test used to get further than it does now, or the
goldens were never cleaned up).

Usage:
    tests/run_tests.py                       # build, then run everything
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

Status = Literal["match", "mismatch", "missing", "timeout", "orphan"]


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


def evaluate_case(case: harness.Case, stage_names: list[str]) -> list[Outcome]:
    """Runs `case` through STAGES (restricted to stage_names) in pipeline
    order, short-circuiting after the first live failure."""
    outcomes: list[Outcome] = []
    stopped = False
    for stage in harness.STAGES:
        if stage.name not in stage_names:
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
            outcomes.append(Outcome(case, stage, "match", result))
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
    "mismatch": "red",
    "missing": "yellow",
    "timeout": "magenta",
    "orphan": "cyan",
}
SYMBOL = {
    "match": "PASS",
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
    """Builds the Rich-markup PASS/FAIL/etc. line for `outcome`, with its
    live run's duration -- or nothing, for an orphan (no live run at all).
    Tags a "match" against a TO_FIX-marked case as a known gap rather than
    genuinely fully optimal; doesn't affect pass/fail, purely cosmetic."""
    label = f"{outcome.case.category}/{outcome.case.name} [{outcome.stage.name}]"
    symbol, style = SYMBOL[outcome.status], STYLE[outcome.status]
    duration = (f" ({format_duration(outcome.result.duration)})"
               if outcome.result is not None else "")
    to_fix = outcome.case.to_fix_reason
    tag = f" [dim]({escape(to_fix)})[/dim]" if outcome.status == "match" and to_fix else ""
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
    parser.add_argument("--quiet", action="store_true", help="only print non-matching results")
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
                if not (args.quiet and outcome.status == "match"):
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
