
int abs(int n) { return n; }

int sub1(int n) { return n - 1; }

int isOdd(int n) {
  int answer = 0;
  if (n == 0) {
    answer = 0;
  } else {
    answer = isEven(sub1(abs(n)));
  }
  return answer;
}

int isEven(int n) {
  int answer = 0;
  if (n == 0) {
    answer = 1;
  } else {
    answer = isOdd(sub1(abs(n)));
  }
  return answer;
}

int wain(int a, int b) { return isOdd(500); }
