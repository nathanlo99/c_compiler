
// ------------ UTIL ------------
// A compiler-optimization-safe way to throw an error
int throw_error() { return *NULL; }

// ------------ IO ------------
int print_char(int c) {
  int *stdout = 0xffff000c;
  return *stdout = c;
}

int print(int *str) {
  int *stdout = 0xffff000c;
  int size = 0;
  int i = 0;
  size = *vector_size(str);
  for (i = 0; i < size; i = i + 1) {
    print_char(*vector_at(str, i));
  }
  return 0;
}

int print_num(int num) {
  int* buffer = NULL;
  int length = 0;
  int i = 0;
  int digit = 0;
  buffer = new int[10];

  if (num < 0) {
    *(buffer + length) = 45; // Minus
    length = length + 1;
    num = 0 - num;
  }

  if (num == 0) {
    *(buffer + length) = 48;
    length = length + 1;
  } else {
    while (num != 0) {
      digit = num % 10;
      *(buffer + length) = digit + 48;
      length = length + 1;
      num = num / 10;
    }
  }

  for (i = length - 1; i >= 0; i = i - 1) {
    print_char(*(buffer + i));
  }
  delete[] buffer;
  return 0;
}

// ------------ VECTOR ------------
// The structure of a dynamic array is the following:
// - The number of elements in the array
// - The capacity of the allocated region
// - The allocated region

int *vector_size(int *vec) { return vec; }
int *vector_capacity(int *vec) { return vec + 1; }
int *vector_data(int *vec) { return vec + 2; }
int *vector_at(int *vec, int idx) { return vector_data(vec) + idx; }

int *vector_new(int n) {
  int *v = NULL;
  if (n < 0) {
    throw_error();
  }
  v = new int[n + 2];
  *vector_size(v) = 0;
  *vector_capacity(v) = n;
  return v;
}

int vector_delete(int *vec) {
  delete[] (vec);
  return 0;
}

int *vector_push_back(int *vec, int value) {
  int *result = NULL;
  int i = 0;
  if (*vector_size(vec) < *vector_capacity(vec)) {
    *vector_at(vec, *vector_size(vec)) = value;
    *vector_size(vec) = *vector_size(vec) + 1;
    result = vec;
  } else {
    result = vector_new(*vector_capacity(vec) * 2);
    i = 0;
    while (i < *vector_size(vec)) {
      *vector_at(result, i) = *vector_at(vec, i);
      i = i + 1;
    }
    *vector_at(result, *vector_size(vec)) = value;
    *vector_size(result) = *vector_size(vec) + 1;
    vector_delete(vec);
  }
  return result;
}

// ------------ MAIN ------------
int wain(int n, int unused) {
  int *vec = NULL;
  int i = 0;

  vec = vector_new(1);
  for (i = 0; i < n; i = i + 1) {
    vec = vector_push_back(vec, i * i);
  }
  for (i = 0; i < n; i = i + 1) {
    if (i > 0) {
      print_char(32);
    }
    print_num(*vector_at(vec, i));
  }
  print_char(10);

  vector_delete(vec);
  return 0;
}
