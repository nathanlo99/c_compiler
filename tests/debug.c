int lex(int *stdin) { return *stdin; }
int printTokens(int *stdin, int *stdout, int *tokens) { return 0; }

int wain(int *stdin, int unused) {
  int *stdout = NULL;
  int *dynarr = NULL;
  int *tokens = NULL;
  int *machine = NULL;
  stdout = stdin + 2;
  // Body
  // Skip terminals
  // unused = skipGarbage(stdin);
  // Skip nonterminals
  // unused = skipGarbage(stdin);
  // Skip start nonterminal
  // tokens = readLine(stdin) + stdin;
  // unused = dynarrDelete(tokens);
  // Skip rules - we'll hardcode them
  // unused = skipGarbage(stdin);
  // Skip number of states - Unneeded
  // unused = readInt(stdin);
  // Read SLR1 machine
  // machine = readMachine(stdin) + stdin;
  tokens = lex(stdin) + stdin;
  unused = printTokens(stdin, stdout, tokens);
  // End
  // delete[] machine;
  return 0;
}
