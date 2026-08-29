
int wain(int a, int b) {
  int i = 0;
  for (i = 0; i < 10; i = i + 1) {
    if (i == 10) {
      break;
    }
    if (i % 2 == 1) {
      continue;
    }
    println(i);
  }
  return i;
}
