
int wain(int a, int b) {
  println(0 + a); // a
  println(a + 0); // a

  println(a - 0); // a
  println(a - a); // 0

  println(a * 1); // a
  println(a * 0); // 0
  println(a * 2); // a + a
  println(1 * a); // a
  println(0 * a); // 0
  println(2 * a); // a + a

  println(0 / a); // 0
  println(a / a); // 1

  println(0 % a); // 0
  println(a % 1); // 0
  println(a % a); // 0

  println(a + b - b); // a

  return a;
}
