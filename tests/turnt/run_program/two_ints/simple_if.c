
int wain(int a, int b) {
  if (b % 2 == 0) {
    a = a * a;
  } else {
    a = a + a;
  }
  println(a);
  return a;
}
