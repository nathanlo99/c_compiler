
int wain(int a, int b) {
  int i = 0;
  for (i = 0; i < 10; i = i + 1) {
    if (i % 2 == 0) {
      continue;
    }
    if (i == 5) {
      break;
    }
  }
  return i;
}
