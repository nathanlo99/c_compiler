int wain(int a, int b) {
  int skip = 0;
  if (a < 0) {
    skip = 1;
  }
  if (b <= a) {
    skip = 1;
  }
  return skip;
}
