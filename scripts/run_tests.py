#!/usr/bin/env python3
"""Convenience wrapper: build, then run the golden-file regression suite.

Superseded turnt (tests/turnt/) as of the tests/cases/ + tests/runner/
reorganization -- see tests/runner/README.md.
"""
import subprocess
import sys

COMMANDS = [
    ["cmake", "--build", "build", "-j"],
    [sys.executable, "tests/runner/run_tests.py"],
]


def main() -> int:
    """Runs each command in COMMANDS in turn, stopping at the first failure."""
    for command in COMMANDS:
        if subprocess.run(command, check=False).returncode != 0:
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
