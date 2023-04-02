int wain(int a, int b) {
  int* addr_a = NULL;
  addr_a = &a;
  b = 0;
  a = 69;
  b = b + a; // b = 69
  *addr_a = 420;
  b = b + a; // b = 489
  return b; // 489 = 0x1e9
}
