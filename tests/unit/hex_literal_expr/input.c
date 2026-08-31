// Regression: hex literals used as an ordinary expression (not just a
// declaration initializer) used to silently parse as 0 -- factor -> NUM
// called std::stoi(lexeme) directly, base 10 only, which stops at 'x'.
int wain(int a, int b) {
  return 0xff + 1;
}
