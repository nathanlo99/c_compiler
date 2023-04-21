
import os
import sys

run_commands = {
    "--twoints": "mips.twoints",
    "--array": "mips.array",
    "--stepper": "mips.stepper_twoints",
    "--stdin": "mips.stdin"
}
options_help_string = "|".join(run_commands.keys())
if len(sys.argv) < 3:
    print("Usage: {program} input.c [{help_string}]".format(
        program=sys.argv[0], help_string=options_help_string))
    sys.exit(1)

input_file = sys.argv[1]
option = sys.argv[2]
compile_mode = "--emit-naive-mips" if len(sys.argv) <= 3 else sys.argv[3]

if option not in run_commands:
    print("Second option should be --array or --twoints")
    sys.exit(1)

run_command = run_commands[option]

commands = [
    "cmake --build build -j",  # Replace this with your compilation command
    "build/compile {} {} > output/output.asm".format(
        input_file, compile_mode),
    "cs241.linkasm < output/output.asm > output/output.merl",
    "cs241.linker output/output.merl references/print.merl references/alloc.merl > output/linked.merl",
    "cs241.merl 0 < output/linked.merl > output/final.mips 2> /dev/null",
    "wc -c output/final.mips",
    "{} output/final.mips".format(run_command)
]

for command in commands:
    os.system(command)
