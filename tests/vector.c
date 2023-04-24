
// A compiler-optimization-safe way to throw an error
int throw_error() {
  println(*NULL);
  return 0;
}

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
  if (n >= 0) {
    v = new int[n + 2];
    *vector_size(v) = 0;
    *vector_capacity(v) = n;
  } else {
    throw_error();
  }
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

int wain(int numElements, int i) {
  int *vec = NULL;
  vec = vector_new(1);
  i = 0;
  while (i < numElements) {
    vec = vector_push_back(vec, i);
    i = i + 1;
  }
  i = 0;
  while (i < numElements) {
    println(*vector_at(vec, i));
    i = i + 1;
  }
  vector_delete(vec);
  return 0;
}
