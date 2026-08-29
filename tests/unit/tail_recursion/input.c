
int odd_part(int n) {
  int result = 0;
  result = n;
  if (n % 2 == 0) {
    result = odd_part(n / 2);
  }
  return result;
}

int wain(int n, int unused) { return odd_part(n); }
