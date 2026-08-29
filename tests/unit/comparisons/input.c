
int wain(int a, int b) {
  if (a < b) {
    println(a);
  }
  // NOTE: This is equivalent to a < b and should be detected as such by GVN
  if (b > a) {
    println(b);
  }
  return a;
}
