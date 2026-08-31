// 2147483648 exceeds int range but isn't a compile error -- WLP4 types
// NUM as `long` (wider than int) and truncates to 32 bits on use, same as
// a real C compiler. Truncated, 2147483648 - 2147483647 == 1.
int wain(int a, int b) {
  return 2147483648 - 2147483647;
}
