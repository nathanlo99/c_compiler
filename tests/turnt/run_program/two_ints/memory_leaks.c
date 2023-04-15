
int wain(int a, int b) {
  // Create 'a' memory leaks, each pf size 'b'
  int i = 0;
  int *ptr = NULL;
  while (i < a) {
    ptr = new int[b];
    i = i + 1;
  }
  return 0;
}
