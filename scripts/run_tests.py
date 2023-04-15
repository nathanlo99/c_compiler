
import os

# What are bash scripts anyway?
commands = [
    "cmake --build build -j8",  # Replace this with your compilation command
    "turnt tests/turnt/build_program/*.c --diff --parallel",
]

for command in commands:
    os.system(command)
