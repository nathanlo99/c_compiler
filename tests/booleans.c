
int wain(int a, int b) {
  int num = 1;
  while (num < a) {
    if (num % 3 == 0 && num % 5 == 0) {
      println(99999);
    } else {
      if (num % 3 == 0) {
        println(333);
      } else {
        if (num % 5 == 0) {
          println(555);
        } else {
          println(num);
        }
      }
    }
    num = num + 1;
  }
  return num;
}
