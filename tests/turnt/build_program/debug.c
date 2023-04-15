
int wain(int n, int b) {
  int cont = 1;
  int i = 0;

  /*
  for (i = 0; i < n; i = i + 1) {
    cont = cont * cont;
  }
  */

  if (n <= 3) {
    cont = 0;
    if (n >= 4) {
      cont = 2;
    }
  }
  return cont;
}
