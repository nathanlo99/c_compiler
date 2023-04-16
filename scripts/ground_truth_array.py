
import sys
import tempfile
import os

template = r"""
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

if len(sys.argv) < 2:
    print("Usage: {} input_file.c".format(sys.argv[0]))
    sys.exit()

contents = open(sys.argv[1], "r").read()

with tempfile.TemporaryDirectory() as directory:
    os.chdir(directory)
    filename = os.path.join(directory, "tmp.cpp")
    with open(filename, "w") as f:
        f.write(template.replace("[REPLACEME]", contents))
    if len(sys.argv) >= 3:
        command = "cat {} | g++ -xc++ - ; rm {} ; ./a.out < {} ; rm a.out".format(
            filename, filename, sys.argv[2])
    else:
        command = "cat {} | g++ -xc++ - ; rm {} ; ./a.out ; rm a.out".format(
            filename, filename)
    os.system(command)
