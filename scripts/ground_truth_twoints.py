
import sys
import tempfile
import os

template = r"""
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

if len(sys.argv) < 2:
    print("Usage: {} input_file.c [input.txt]".format(sys.argv[0]))
    sys.exit()

contents = open(sys.argv[1], "r").read()
cpp_source = template.replace("[REPLACEME]", contents)

with tempfile.TemporaryDirectory() as directory:
    os.chdir(directory)
    filename = os.path.join(directory, "tmp.cpp")
    with open(filename, "w") as f:
        f.write(cpp_source)
    command = "cat {} | g++ -xc++ - ; rm {} ; ./a.out".format(
        filename, filename)
    os.system(command)
