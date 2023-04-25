
int isOdd(int n) {
  int answer = 0;
  if (n == 0) {
    answer = 0;
  } else {
    answer = isEven(n - 1);
  }
  return answer;
}

int isEven(int n) {
  int answer = 0;
  if (n == 0) {
    answer = 1;
  } else {
    answer = isOdd(n - 1);
  }
  return answer;
}

int wain(int a, int b) { return isOdd(500) * isEven(500); }
