
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
  printf("Enter length of array: ");
  scanf("%d", &l);
  a = (int*) malloc(l*sizeof(int));
  for(int i = 0; i < l; i++) {
    printf("Enter value of array element %d: ", i);
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
open("tmp.cpp", "w").write(template.replace("[REPLACEME]", contents))

if len(sys.argv) >= 3:
    command = "cat tmp.cpp | g++ -xc++ - && rm tmp.cpp && ./a.out < {input_file} && rm a.out".format(
        input_file=sys.argv[2])
else:
    command = "cat tmp.cpp | g++ -xc++ - && rm tmp.cpp && ./a.out && rm a.out"
os.system(command)
