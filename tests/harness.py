"""Shared logic for the regression-test / golden-file tooling.

Layout (one directory per test case):

    tests/<category>/<name>/
        input.c        - the C program under test; may start with a
                          `// test phases: <name>, <name>, ...` comment
                          (see Case.declared_phases) naming a subset of
                          STAGES to run instead of DEFAULT_STAGE_NAMES
        stdin           - (optional) stdin fed to the "interpret" stage;
                          defaults to "5 10\\n" (two ints) if absent
        TO_FIX          - (optional) marks this as a known, deliberate
                          repro for a references/TODO.md item (names
                          which); absent means "expected fully optimal"
        <stage>.out     - golden stdout (or stderr, if the stage failed
                          and stdout was empty) for that stage

A case's stages are evaluated in STAGES order, restricted to whichever
subset applies to it (see Case.stage_names). The moment a stage's *live*
run returns a non-zero exit code, evaluation for that test case stops:
later stages are not run, not checked against golden files, and should
not have golden files at all (a test that's meant to fail to parse simply
has no mips/interpret golden -- absence, not an empty file, is how
"doesn't get this far" is represented).

Return codes aren't golden-checked -- in practice every non-zero-rc case
we've hit also changed its stdout, so the text diff alone catches it, and
storing/comparing rc separately wasn't pulling its weight. The live rc
still matters at runtime (`RunResult.failed` drives short-circuiting), it's
just not persisted or diffed.
"""
from __future__ import annotations

import dataclasses
import pathlib
import re
import subprocess
import time

REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
COMPILE_BIN = REPO_ROOT / "build" / "compile"
CASES_DIR = REPO_ROOT / "tests"

CATEGORIES = ["unit", "benchmarks", "broken"]

DEFAULT_STDIN = b"5 10\n"
DEFAULT_TIMEOUT = 15.0


@dataclasses.dataclass(frozen=True)
class Stage:
    """One point in the compiler pipeline we can check golden output for,
    e.g. ("bril", "--bril")."""
    name: str
    cli_arg: str
    needs_stdin: bool = False


# Order matters: this is the canonical order stages run/print in. ast/bril
# were dropped -- we're not developing the frontend right now, and the
# optimizer/codegen work this suite tracks (see references/TODO.md) is
# about "optimized" and "mips", not the raw pre-optimization IR.
STAGES: list[Stage] = [
    Stage("optimized", "--run-optimizations"),
    Stage("mips", "--emit-mips"),
    Stage("interpret", "--interpret", needs_stdin=True),
    Stage("compute-rig", "--compute-rig"),
]
STAGES_BY_NAME = {s.name: s for s in STAGES}

# What a case runs if its input.c has no `// test phases:` comment. Doesn't
# include "compute-rig": it's opt-in per case (see Case.declared_phases)
# since almost no test cares about register-interference-graph output, and
# defaulting it on would mean a golden file for every single case.
DEFAULT_STAGE_NAMES = ["optimized", "mips", "interpret"]

_PHASES_COMMENT_RE = re.compile(r"^//\s*test phases:\s*(.+)$", re.MULTILINE)


@dataclasses.dataclass(frozen=True)
class Case:
    """A single test case: `tests/<category>/<name>/`."""
    category: str
    name: str
    dir: pathlib.Path

    @property
    def c_path(self) -> pathlib.Path:
        """Path to this case's C source file."""
        return self.dir / "input.c"

    @property
    def stdin_path(self) -> pathlib.Path:
        """Path to this case's optional stdin override file."""
        return self.dir / "stdin"

    def stdin_bytes(self) -> bytes:
        """This case's stdin for the "interpret" stage: its `stdin` file
        if present, else DEFAULT_STDIN."""
        if self.stdin_path.exists():
            return self.stdin_path.read_bytes()
        return DEFAULT_STDIN

    @property
    def declared_phases(self) -> list[str] | None:
        """The stage names named by input.c's `// test phases: ...`
        comment, in the order given, or None if it has no such comment
        (in which case DEFAULT_STAGE_NAMES applies -- see stage_names).
        Raises ValueError if a named phase isn't a known stage, to catch
        typos loudly instead of silently skipping a stage."""
        match = _PHASES_COMMENT_RE.search(self.c_path.read_text(encoding="utf-8"))
        if match is None:
            return None
        names = [name.strip() for name in match.group(1).split(",")]
        for name in names:
            if name not in STAGES_BY_NAME:
                raise ValueError(
                    f"{self.c_path}: unknown phase {name!r} in 'test phases' "
                    f"comment (known: {', '.join(STAGES_BY_NAME)})"
                )
        return names

    def stage_names(self) -> list[str]:
        """The stage names this case actually runs: its own declared_phases
        if it has any, else DEFAULT_STAGE_NAMES."""
        return self.declared_phases or DEFAULT_STAGE_NAMES

    @property
    def to_fix_path(self) -> pathlib.Path:
        """Path to this case's optional TO_FIX marker."""
        return self.dir / "TO_FIX"

    @property
    def to_fix_reason(self) -> str | None:
        """Why this case is deliberately not fully optimal yet (its
        TO_FIX file's first line, naming a references/TODO.md item), or
        None if it's not marked -- i.e. it's expected to be fully
        optimal."""
        if not self.to_fix_path.exists():
            return None
        lines = self.to_fix_path.read_text(encoding="utf-8").splitlines()
        return lines[0] if lines else ""


@dataclasses.dataclass
class RunResult:
    """The outcome of running one Stage of one Case, live or from golden.
    `duration` is wall-clock seconds for a live run, 0.0 for a golden
    (golden files don't record timing, and it's not part of the
    match/mismatch comparison -- purely informational for display)."""
    returncode: int
    stdout: str
    stderr: str
    timed_out: bool = False
    duration: float = 0.0

    @property
    def failed(self) -> bool:
        """True if this stage didn't complete successfully (whether it
        timed out or just returned non-zero)."""
        return self.timed_out or self.returncode != 0

    @property
    def golden_text(self) -> str:
        """What we'd store/compare as the golden ".out" content: stdout
        normally, but fall back to stderr for a failing stage whose stdout
        is empty (e.g. a parse error, which main.cpp prints to stderr)."""
        if self.returncode != 0 and not self.stdout.strip():
            return self.stderr
        return self.stdout


def discover_cases(categories: list[str] | None = None,
                    pattern: str | None = None) -> list[Case]:
    """Finds every test case directory (one with an input.c) under the
    given categories (default: all of them), optionally filtered to names
    containing `pattern`."""
    cases = []
    for category in categories or CATEGORIES:
        cat_dir = CASES_DIR / category
        if not cat_dir.is_dir():
            continue
        for test_dir in sorted(cat_dir.iterdir()):
            if not test_dir.is_dir():
                continue
            if not (test_dir / "input.c").exists():
                continue
            if pattern and pattern not in test_dir.name:
                continue
            cases.append(Case(category=category, name=test_dir.name, dir=test_dir))
    return cases


def build_compiler() -> int:
    """Runs `cmake --build`, streaming its output. Returns its exit code."""
    result = subprocess.run(["cmake", "--build", str(REPO_ROOT / "build"), "-j"], check=False)
    return result.returncode


def run_stage(case: Case, stage: Stage, timeout: float = DEFAULT_TIMEOUT) -> RunResult:
    """Runs `case` through one pipeline `stage` live and captures the
    result. Raises FileNotFoundError if the compiler hasn't been built."""
    if not COMPILE_BIN.exists():
        raise FileNotFoundError(
            f"compiler binary not found at {COMPILE_BIN}; build it first "
            f"(cmake --build build)"
        )
    stdin_data = case.stdin_bytes() if stage.needs_stdin else b""
    start = time.perf_counter()
    try:
        proc = subprocess.run(
            [str(COMPILE_BIN), str(case.c_path), stage.cli_arg],
            input=stdin_data,
            capture_output=True,
            timeout=timeout,
            check=False,
        )
    except subprocess.TimeoutExpired as exc:
        return RunResult(
            returncode=-1,
            stdout=(exc.stdout or b"").decode("utf-8", "replace"),
            stderr=(exc.stderr or b"").decode("utf-8", "replace"),
            timed_out=True,
            duration=time.perf_counter() - start,
        )
    return RunResult(
        returncode=proc.returncode,
        stdout=proc.stdout.decode("utf-8", "replace"),
        stderr=proc.stderr.decode("utf-8", "replace"),
        duration=time.perf_counter() - start,
    )


def golden_paths(case: Case, stage: Stage) -> pathlib.Path:
    """The golden output file path for `case`'s `stage`."""
    return case.dir / f"{stage.name}.out"


def read_golden(case: Case, stage: Stage) -> str | None:
    """The checked-in golden text for `case`'s `stage`, or None if there
    isn't one yet."""
    out_path = golden_paths(case, stage)
    if not out_path.exists():
        return None
    return out_path.read_text(encoding="utf-8")


def write_golden(case: Case, stage: Stage, result: RunResult) -> None:
    """Writes `result`'s golden text as the new golden output for `case`'s
    `stage`."""
    out_path = golden_paths(case, stage)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(result.golden_text, encoding="utf-8")


def clear_golden(case: Case, stage: Stage) -> None:
    """Remove a stage's golden file, e.g. because an earlier stage now
    fails and this stage is no longer reachable."""
    golden_paths(case, stage).unlink(missing_ok=True)
