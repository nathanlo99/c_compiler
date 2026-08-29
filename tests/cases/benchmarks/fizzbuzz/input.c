
int wain(int start, int end) {
  int idx = 0;
  idx = start;

  while (idx <= end) {
    if (idx % 3 == 0) {
      if (idx % 5 == 0) {
        println(0 - 15);
      } else {
        println(0 - 3);
      }
    } else {
      if (idx % 5 == 0) {
        println(0 - 5);
      } else {
        println(idx);
      }
    }

    idx = idx + 1;
  }

  return idx;
}
