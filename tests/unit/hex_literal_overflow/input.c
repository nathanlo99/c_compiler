// 0x100000000 (2^32) truncates to 0, same as a real C compiler narrowing
// it to int. 0 - 1 == -1.
int wain(int a, int b) {
  return 0x100000000 - 1;
}
