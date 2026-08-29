
int print_char(int c) {
  int *stdout = 0xffff000c;
  return *stdout = c;
}

int print_num(int num) {
  int *stdout = 0xffff000c;
  int *buffer = NULL;
  int length = 0;
  int i = 0;
  int digit = 0;
  buffer = new int[10];

  if (num == 0) {
    print_char(48);
  }
  if (num < 0) {
    print_char(45);
    num = 0 - num;
  }
  while (num != 0) {
    digit = num % 10;
    *(buffer + length) = digit + 48;
    length = length + 1;
    num = num / 10;
  }

  for (i = length - 1; i >= 0; i = i - 1) {
    print_char(*(buffer + i));
  }

  delete[] buffer;
  return 0;
}

int wain(int a, int b) {
  print_num(a);
  print_char(32);
  print_char(43);
  print_char(32);
  print_num(b);
  print_char(32);
  print_char(61);
  print_char(32);
  print_num(a + b);
  print_char(10);
  return 0;
}
