#!/usr/bin/env python3
"""Ground-truth oracle for a test .c file: wraps its `wain` in a real
`main()`, compiles it with a real C++ compiler, and runs it -- so its
output can be compared against the BRIL interpreter / MIPS execution for
the same file.

`wain`'s signature (`wain(int, int)` vs `wain(int*, int)`) is inferred from
the source. The compiled program's stdin is just this script's own stdin --
pipe or redirect input the same way you would for the compiled binary itself.

Usage:
    ground_truth.py input_file.c
    ground_truth.py input_file.c < input.txt
"""
from __future__ import annotations

import enum
import pathlib
import re
import subprocess
import sys
import tempfile

import click

TWO_INTS_TEMPLATE = r"""
int wain(int, int);
void println(int);
#define NULL 0

[REPLACEME]

#include <stdlib.h>
#include <stdio.h>
int main(int argc, char** argv) {
  int a,b,c;
  fprintf(stderr, "Enter first integer: ");
  scanf("%d", &a);
  fprintf(stderr, "Enter second integer: ");
  scanf("%d", &b);
  c = wain(a,b);
  printf("wain returned %d\n", c);
  return 0;
}
void println(int x){
   printf("%d\n",x);
}"""

ARRAY_TEMPLATE = r"""
int wain(int*, int);
void println(int);
#define NULL 0

[REPLACEME]

#include <stdlib.h>
#include <stdio.h>
int main(int argc, char** argv) {
  int l, c;
  int* a;
  fprintf(stderr, "Enter length of array: ");
  scanf("%d", &l);
  a = (int*) malloc(l*sizeof(int));
  for(int i = 0; i < l; i++) {
    fprintf(stderr, "Enter value of array element %d: ", i);
    scanf("%d", a+i);
  }
  c = wain(a,l);
  printf("wain returned %d\n", c);
  return 0;
}
void println(int x){
   printf("%d\n",x);
}"""


class WainSignature(enum.Enum):
    """Which `wain` overload a test .c file declares."""
    TWO_INTS = enum.auto()
    ARRAY = enum.auto()


TEMPLATES = {
    WainSignature.TWO_INTS: TWO_INTS_TEMPLATE,
    WainSignature.ARRAY: ARRAY_TEMPLATE,
}

# Matches wain's actual *definition* -- a parameter list immediately
# followed by `{` -- not a mere prototype/declaration or a comment that
# happens to mention "wain(int...". Searching the whole file for either
# signature's pattern independently is what let a stray prototype or an
# offhand comment about "the other" signature register as a false match;
# anchoring to one specific definition avoids that.
WAIN_DEFINITION_RE = re.compile(r"\bwain\s*\(([^)]*)\)\s*\{")

# Checked against the definition's captured parameter list (not the whole
# file), so these stay mutually exclusive by construction: the lookahead
# means at most one can match a given "int a, ..." vs "int *a, ...".
TWO_INTS_RE = re.compile(r"^\s*int\b(?!\s*\*)")
ARRAY_RE = re.compile(r"^\s*int\s*\*")


def detect_signature(source: str, path: pathlib.Path) -> WainSignature | None:
    """Finds wain's actual definition in `source` and guesses its
    signature from the first parameter's type. Returns None (after
    printing a warning naming `path`, for the caller to act on) if no
    definition was found, or its first parameter didn't look like `int`
    or `int*`."""
    if (match := WAIN_DEFINITION_RE.search(source)) is None:
        click.echo(f"warning: couldn't find a definition of wain in {path}", err=True)
        return None
    params = match.group(1)
    is_two_ints = TWO_INTS_RE.match(params) is not None
    is_array = ARRAY_RE.match(params) is not None
    if is_two_ints != is_array:
        return WainSignature.TWO_INTS if is_two_ints else WainSignature.ARRAY
    click.echo(f"warning: found wain({params.strip()}, ...) in {path} but "
              "couldn't tell if its first parameter is `int` or `int*`", err=True)
    return None


def compile_and_run(source: str) -> int:
    """Compiles `source` with g++ into a temporary binary and runs it,
    inheriting this process's own stdin. Returns the exit code of the
    compile step (if it fails) or of the run step."""
    with tempfile.TemporaryDirectory() as directory_str:
        directory = pathlib.Path(directory_str)
        binary = directory / "a.out"

        compile_result = subprocess.run(
            ["g++", "-xc++", "-", "-o", str(binary)],
            input=source, text=True, check=False,
        )
        if compile_result.returncode != 0:
            return compile_result.returncode

        run_result = subprocess.run([str(binary)], check=False)
        return run_result.returncode


@click.command()
@click.argument("input_file", type=click.Path(exists=True, dir_okay=False, path_type=pathlib.Path))
def main(input_file: pathlib.Path) -> None:
    """Wraps INPUT_FILE's `wain` in a template inferred from its signature,
    compiles it, and runs it against this process's own stdin."""
    contents = input_file.read_text(encoding="utf-8")
    if (signature := detect_signature(contents, input_file)) is None:
        sys.exit(1)
    click.echo(f"(inferred: {signature.name})", err=True)

    source = TEMPLATES[signature].replace("[REPLACEME]", contents)
    sys.exit(compile_and_run(source))


if __name__ == "__main__":
    main()  # pylint: disable=no-value-for-parameter  # click parses sys.argv itself
