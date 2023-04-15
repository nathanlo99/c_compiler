
int wain(int n, int b) {
  int cont = 1;

  if (n <= 3) {
    cont = 0;
    if (n >= 4) {
      cont = 2;
    }
  }
  return cont;
}
