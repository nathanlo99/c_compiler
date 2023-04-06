// Must be run with mips-io.
// standard input should contain the WLP4 machine
// and then the program

// I/O Functions
int print(int *stdout, int *msg) {
  while (*msg != 0) {
    *stdout = *msg;
    msg = msg + 1;
  }
  return 0;
}
int printline(int *stdout, int *msg) {
  int t = 0;
  t = print(stdout, msg);
  *stdout = 10;
  return 0;
}

// Crashes the program by dereferencing NULL
int error() {
  int *nil = NULL;
  *nil = 5;
  return *nil;
}

// String functions
int strcmp(int *a, int *b) {
  int diff = 1;
  int cond = 0;
  while (cond == 0) {
    if (*a != *b) {
      cond = 1;
    } else {
    }
    if (*a == 0) {
      cond = 1;
    } else {
    }
    if (*b == 0) {
      cond = 1;
    } else {
    }
    if (cond == 0) {
      diff = diff + 1;
      a = a + 1;
      b = b + 1;
    } else {
    }
  }
  if (*a == 0) {
    if (*b == 0) {
      diff = 0;
    } else {
    }
  } else {
  }
  return diff;
}

// Destructive!
int toupper(int *a) {
  while (*a != 0) {
    if (*a > 96) {
      if (*a < 123) {
        *a = *a - 32;
      } else {
      }
    } else {
    }
    a = a + 1;
  }
  return 0;
}

// Reads an int from standard input
// Assumes the int is the only thing on the line
// Intended to be used with the .lr file format
int readInt(int *stdin) {
  int resl = 0;
  int nextC = 0;
  nextC = *stdin;
  while (nextC != 10) {
    if (nextC == 0 - 1) {
      resl = error();
    } else {
    }
    nextC = nextC - 48;
    if (nextC < 0) {
      resl = error();
    } else {
    }
    if (nextC > 9) {
      resl = error();
    } else {
    }
    resl = resl * 10 + nextC;
  }
  return resl;
}

// WLP4 Dynamic Arrays
// The dynamic array starts 2 words BEFORE the pointer!
// Dynamic array memory layout:
// size
// capacity
//<elements>

int dynarrSize(int *dynarr) { return *(dynarr - 2); }
int dynarrDelete(int *dynarr) {
  delete[] (dynarr - 2);
  return 0;
}
int dynarrNew(int *ref) {
  int *dynarr = NULL;
  dynarr = new int[2];
  *dynarr = 0;
  *(dynarr + 1) = 0;
  dynarr = dynarr + 2;
  return dynarr - ref;
}
// Makes a dynamic version of an array. Intended for strings
// as an efficiency/convenience improvement.
int dynarrNewString(int *arr, int len) {
  int *dynarr = NULL;
  int *dynarrP = NULL;
  int *oarr = NULL;
  oarr = arr;
  dynarr = new int[len + 3];
  *dynarr = len + 1;
  *(dynarr + 1) = len + 1;
  dynarr = dynarr + 2;
  dynarrP = dynarr;
  while (len > 0) {
    *dynarr = *arr;
    dynarr = dynarr + 1;
    arr = arr + 1;
    len = len - 1;
  }
  *dynarr = 0;
  return dynarrP - oarr;
}
int dynarrPush(int *dynarr, int elem) {
  int capacity = 0;
  int size = 0;
  int *outdynarr = NULL;
  int i = 0;
  capacity = *(dynarr - 1);
  size = *(dynarr - 2);

  if (capacity == size) {
    if (capacity == 0) {
      capacity = 5;
      outdynarr = new int[capacity + 2];
      outdynarr = outdynarr + 2;
    } else {
      capacity = capacity * 2;
      outdynarr = new int[capacity + 2];
      outdynarr = outdynarr + 2;
      i = 0;
      while (i < size) {
        *(outdynarr + i) = *(dynarr + i);
        i = i + 1;
      }
    }
    delete[] (dynarr - 2);
  } else {
    outdynarr = dynarr;
  }
  *(outdynarr + size) = elem;
  size = size + 1;
  *(outdynarr - 2) = size;
  *(outdynarr - 1) = capacity;
  return outdynarr - dynarr;
}

// WLP4 Trees
// Arrays of size 2
// Element 1: Pointer to node contents dynamic array
// Element 2: Pointer to children dynamic array
int treeNew(int *ref, int *node) {
  int *tree = NULL;
  tree = new int[2];
  *tree = node - tree;
  *(tree + 1) = dynarrNew(tree);
  return tree - ref;
}
int treeDelete(int *tree) {
  int children = 0;
  int numChildren = 0;
  int *dynarr = NULL;
  int t = 0;
  dynarr = *(tree + 1) + tree;
  numChildren = *(dynarr - 2);
  while (children < numChildren) {
    t = treeDelete(*(dynarr + children) + tree);
    children = children + 1;
  }
  numChildren = dynarrDelete(*tree + tree);
  numChildren = dynarrDelete(dynarr);
  delete[] tree;
  return 0;
}
int treeAddChild(int *tree, int *child) {
  int *dynarr = NULL;
  dynarr = *(tree + 1) + tree;
  dynarr = dynarrPush(dynarr, child - tree) + dynarr;
  *(tree + 1) = dynarr - tree;
  return 0;
}
int treeGetChild(int *tree, int index) {
  int *dynarr = NULL;
  dynarr = *(tree + 1) + tree;
  return *(dynarr + index);
}
int treeGetNode(int *tree) { return *tree; }

// IO part 2
// Uses dynamic arrays as strings.

// Reads a line from standard input
// Returns it stored in a dynamic array
int readLine(int *stdin) {
  int *dynarr = NULL;
  int chr = 0;
  dynarr = dynarrNew(stdin) + stdin;
  chr = *stdin;
  while (chr != 10) {
    if (chr == 0 - 1) {
      chr = error();
    } else {
    }
    dynarr = dynarrPush(dynarr, chr) + dynarr;
    chr = *stdin;
  }
  return dynarr - stdin;
}

// Garbage skipper in lr file
int skipGarbage(int *stdin) {
  int lines = 0;
  int *t = NULL;
  int te = 0;
  lines = readInt(stdin);
  while (lines > 0) {
    t = readLine(stdin) + stdin;
    te = dynarrDelete(t);
    lines = lines - 1;
  }
  return 0;
}

// Reads the LR1 machine into a hashtable
int readMachine(int *stdin) {
  int *machine = NULL;
  int moves = 0;
  int *line = NULL;
  int *linea = NULL;
  int t = 0;
  moves = readInt(stdin);
  machine = new int[moves + 1];
  *machine = moves;
  while (moves > 0) {
    line = readLine(stdin) + stdin;
    linea = new int[4];
    // Unimplemented: TODO
    //*linea = strReadInt(line);
    t = dynarrDelete(line);
    moves = moves - 1;
  }
  return machine - stdin;
}

int tokensPush(int *stdin, int *tokens, int *curStr, int *curType) {
  int *ntokens = NULL;
  int *tarr = NULL;
  if (curType != NULL) {
    if (curStr == NULL) {
      *ntokens = error();
    } else {
    }
    tarr = new int[2];
    *tarr = curType - tarr;
    *(tarr + 1) = curStr - tarr;
    ntokens = dynarrPush(tokens, tarr - stdin) + tokens;
  } else {
  }
  return ntokens - tokens;
}

// Scanner
int lex(int *stdin) {
  int chr = 0;
  int *tokens = NULL;
  int *curStr = NULL;
  int *curType = NULL;
  int *tarr = NULL;
  int matchingState = 0;
  int lchr = 0;
  int advance = 1;
  tokens = dynarrNew(stdin) + stdin;
  chr = *stdin;
  while (chr != 0 - 1) {
    // State 0: match anything
    // and figure out what to do with it
    if (matchingState == 0) {
      lchr = 0;

      // Characters which are stand-alone tokens
      //%
      if (chr == 37) {
        lchr = 1;
        tarr = new int[3];
        *tarr = 80;
        *(tarr + 1) = 67;
        *(tarr + 2) = 84;
        curType = dynarrNewString(tarr, 3) + tarr;
        *tarr = 37;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
      //&
      if (chr == 38) {
        lchr = 1;
        tarr = new int[3];
        *tarr = 65;
        *(tarr + 1) = 77;
        *(tarr + 2) = 80;
        curType = dynarrNewString(tarr, 3) + tarr;
        *tarr = 38;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
      //(
      if (chr == 40) {
        lchr = 1;
        tarr = new int[6];
        *tarr = 76;
        *(tarr + 1) = 80;
        *(tarr + 2) = 65;
        *(tarr + 3) = 82;
        *(tarr + 4) = 69;
        *(tarr + 5) = 78;
        curType = dynarrNewString(tarr, 6) + tarr;
        *tarr = 40;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
      //)
      if (chr == 41) {
        lchr = 1;
        tarr = new int[6];
        *tarr = 82;
        *(tarr + 1) = 80;
        *(tarr + 2) = 65;
        *(tarr + 3) = 82;
        *(tarr + 4) = 69;
        *(tarr + 5) = 78;
        curType = dynarrNewString(tarr, 6) + tarr;
        *tarr = 41;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
      //*
      if (chr == 42) {
        lchr = 1;
        tarr = new int[5];
        *tarr = 84;
        *(tarr + 1) = 73;
        *(tarr + 2) = 77;
        *(tarr + 3) = 69;
        *(tarr + 4) = 83;
        curType = dynarrNewString(tarr, 5) + tarr;
        *tarr = 42;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
      //+
      if (chr == 43) {
        lchr = 1;
        tarr = new int[4];
        *tarr = 80;
        *(tarr + 1) = 76;
        *(tarr + 2) = 85;
        *(tarr + 3) = 83;
        curType = dynarrNewString(tarr, 4) + tarr;
        *tarr = 43;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
      //,
      if (chr == 44) {
        lchr = 1;
        tarr = new int[5];
        *tarr = 67;
        *(tarr + 1) = 79;
        *(tarr + 2) = 77;
        *(tarr + 3) = 77;
        *(tarr + 4) = 65;
        curType = dynarrNewString(tarr, 5) + tarr;
        *tarr = 44;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
      //-
      if (chr == 45) {
        lchr = 1;
        tarr = new int[5];
        *tarr = 77;
        *(tarr + 1) = 73;
        *(tarr + 2) = 78;
        *(tarr + 3) = 85;
        *(tarr + 4) = 83;
        curType = dynarrNewString(tarr, 5) + tarr;
        *tarr = 45;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
      //;
      if (chr == 59) {
        lchr = 1;
        tarr = new int[4];
        *tarr = 83;
        *(tarr + 1) = 69;
        *(tarr + 2) = 77;
        *(tarr + 3) = 73;
        curType = dynarrNewString(tarr, 4) + tarr;
        *tarr = 59;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
      //[
      if (chr == 91) {
        lchr = 1;
        tarr = new int[6];
        *tarr = 76;
        *(tarr + 1) = 66;
        *(tarr + 2) = 82;
        *(tarr + 3) = 65;
        *(tarr + 4) = 67;
        *(tarr + 5) = 75;
        curType = dynarrNewString(tarr, 6) + tarr;
        *tarr = 91;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
      //]
      if (chr == 93) {
        lchr = 1;
        tarr = new int[6];
        *tarr = 82;
        *(tarr + 1) = 66;
        *(tarr + 2) = 82;
        *(tarr + 3) = 65;
        *(tarr + 4) = 67;
        *(tarr + 5) = 75;
        curType = dynarrNewString(tarr, 6) + tarr;
        *tarr = 93;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
      //{
      if (chr == 123) {
        lchr = 1;
        tarr = new int[6];
        *tarr = 76;
        *(tarr + 1) = 66;
        *(tarr + 2) = 82;
        *(tarr + 3) = 65;
        *(tarr + 4) = 67;
        *(tarr + 5) = 69;
        curType = dynarrNewString(tarr, 6) + tarr;
        *tarr = 123;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
      //}
      if (chr == 125) {
        lchr = 1;
        tarr = new int[6];
        *tarr = 82;
        *(tarr + 1) = 66;
        *(tarr + 2) = 82;
        *(tarr + 3) = 65;
        *(tarr + 4) = 67;
        *(tarr + 5) = 69;
        curType = dynarrNewString(tarr, 6) + tarr;
        *tarr = 125;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }

      // Characters which start medium-sized tokens
      //!
      if (chr == 33) {
        lchr = 33;
        matchingState = 0 - 1;
      } else {
      }
      //<
      if (chr == 60) {
        lchr = 61;
        matchingState = 0 - 1;
      } else {
      }
      //=
      if (chr == 61) {
        lchr = 61;
        matchingState = 0 - 1;
      } else {
      }
      //>
      if (chr == 62) {
        lchr = 62;
        matchingState = 0 - 1;
      } else {
      }

      ///
      if (chr == 47) {
        lchr = 1;
        matchingState = 0 - 2;
      } else {
      }

      // 0
      if (chr == 48) {
        lchr = 1;
        matchingState = 0 - 4;
      } else {
      }

      // 1-9
      if (chr < 58) {
        if (chr > 48) {
          lchr = 1;
          matchingState = 0 - 5;
          curStr = dynarrNew(stdin) + stdin;
          curStr = dynarrPush(curStr, chr) + curStr;
        } else {
        }
      } else {
      }

      // A-Z
      if (chr > 64) {
        if (chr < 91) {
          lchr = 1;
          matchingState = 0 - 6;
          curStr = dynarrNew(stdin) + stdin;
          curStr = dynarrPush(curStr, chr) + curStr;
        } else {
        }
      } else {
      }
      // a-z
      if (chr > 96) {
        if (chr < 123) {
          lchr = 1;
          matchingState = 0 - 6;
          curStr = dynarrNew(stdin) + stdin;
          curStr = dynarrPush(curStr, chr) + curStr;
        } else {
        }
      } else {
      }

      // Error on others
      if (lchr == 0) {
        lchr = error();
      } else {
      }
    } else {
    }
    // State 1: Expect an = next
    if (matchingState == 1) {
      matchingState = 0;
      if (chr == 61) {
        tarr = new int[2];
        //!
        if (lchr == 33) {
          *tarr = 78;
          *(tarr + 1) = 69;
        } else {
        }
        //<
        if (lchr == 60) {
          *tarr = 76;
          *(tarr + 1) = 69;
        } else {
        }
        //=
        if (lchr == 61) {
          *tarr = 69;
          *(tarr + 1) = 81;
        } else {
        }
        //>
        if (lchr == 62) {
          *tarr = 71;
          *(tarr + 1) = 69;
        } else {
        }
        curType = dynarrNewString(tarr, 2) + tarr;
        *tarr = lchr;
        *(tarr + 1) = lchr;
        curStr = dynarrNewString(tarr, 2) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
        advance = 0;
        //!
        if (lchr == 33) {
          lchr = error();
        } else {
        }
        //=
        if (lchr == 61) {
          tarr = new int[7];
          *tarr = 66;
          *(tarr + 1) = 69;
          *(tarr + 2) = 67;
          *(tarr + 3) = 79;
          *(tarr + 4) = 77;
          *(tarr + 5) = 69;
          *(tarr + 6) = 83;
          curType = dynarrNewString(tarr, 7) + tarr;
        } else {
          tarr = new int[2];
          //<
          if (lchr == 60) {
            *tarr = 76;
            *(tarr + 1) = 84;
          } else {
          }
          //>
          if (lchr == 62) {
            *tarr = 71;
            *(tarr + 1) = 84;
          } else {
          }
          curType = dynarrNewString(tarr, 2) + tarr;
        }
        *tarr = lchr;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      }
    } else {
    }
    // State 2: Possible start of comment
    if (matchingState == 2) {
      if (chr == 47) {
        matchingState = 0 - 3;
      } else {
        matchingState = 0;
        advance = 0;
        tarr = new int[5];
        *tarr = 83;
        *(tarr + 1) = 76;
        *(tarr + 2) = 65;
        *(tarr + 3) = 83;
        *(tarr + 4) = 72;
        curType = dynarrNewString(tarr, 5) + tarr;
        *tarr = 47;
        curStr = dynarrNewString(tarr, 1) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      }
    } else {
    }
    // State 3: line comment
    if (matchingState == 3) {
      if (chr == 10) {
        matchingState = 0;
      } else {
      }
    } else {
    }
    // State 4: Zero
    if (matchingState == 4) {
      if (chr > 47) {
        if (chr < 58) {
          chr = error();
        } else {
        }
      } else {
      }
      if (chr > 64) {
        if (chr < 91) {
          lchr = error();
        } else {
        }
      } else {
      }
      if (chr > 96) {
        if (chr < 123) {
          lchr = error();
        } else {
        }
      } else {
      }
      matchingState = 0;
      advance = 0;
      tarr = new int[4];
      *tarr = 90;
      *(tarr + 1) = 69;
      *(tarr + 2) = 82;
      *(tarr + 3) = 79;
      curType = dynarrNewString(tarr, 4) + tarr;
      *tarr = 48;
      curStr = dynarrNewString(tarr, 1) + tarr;
      delete[] tarr;
      tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
    } else {
    }
    // State 5: Number
    if (matchingState == 5) {
      lchr = 0;
      if (chr > 47) {
        if (chr < 58) {
          lchr = 1;
          curStr = dynarrPush(curStr, chr) + curStr;
        } else {
        }
      } else {
      }
      if (chr > 64) {
        if (chr < 91) {
          lchr = error();
        } else {
        }
      } else {
      }
      if (chr > 96) {
        if (chr < 123) {
          lchr = error();
        } else {
        }
      } else {
      }
      if (lchr == 0) {
        matchingState = 0;
        advance = 0;
        tarr = new int[3];
        *tarr = 78;
        *(tarr + 1) = 85;
        *(tarr + 2) = 77;
        curStr = dynarrPush(curStr, 0) + curStr;
        curType = dynarrNewString(tarr, 3) + tarr;
        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
    } else {
    }
    if (matchingState == 6) {
      lchr = 0;
      if (chr > 47) {
        if (chr < 58) {
          lchr = 1;
          curStr = dynarrPush(curStr, chr) + curStr;
        } else {
        }
      } else {
      }
      if (chr > 64) {
        if (chr < 91) {
          lchr = 1;
          curStr = dynarrPush(curStr, chr) + curStr;
        } else {
        }
      } else {
      }
      if (chr > 96) {
        if (chr < 123) {
          lchr = 1;
          curStr = dynarrPush(curStr, chr) + curStr;
        } else {
        }
      } else {
      }
      if (lchr == 0) {
        matchingState = 0;
        advance = 0;
        curStr = dynarrPush(curStr, 0) + curStr;
        tarr = new int[8];
        // Determine if the type is ID or a special type
        curType = NULL;

        // int
        *tarr = 105;
        *(tarr + 1) = 110;
        *(tarr + 2) = 116;
        *(tarr + 3) = 0;
        if (strcmp(curStr, tarr) == 0) {
          lchr = toupper(tarr);
          curType = dynarrNewString(tarr, 3) + tarr;
        } else {
        }

        // wain
        *tarr = 119;
        *(tarr + 1) = 97;
        *(tarr + 2) = 105;
        *(tarr + 3) = 110;
        *(tarr + 4) = 0;
        if (strcmp(curStr, tarr) == 0) {
          lchr = toupper(tarr);
          curType = dynarrNewString(tarr, 4) + tarr;
        } else {
        }

        // NULL
        *tarr = 78;
        *(tarr + 1) = 85;
        *(tarr + 2) = 76;
        *(tarr + 3) = 76;
        *(tarr + 4) = 0;
        if (strcmp(curStr, tarr) == 0) {
          curType = dynarrNewString(tarr, 4) + tarr;
        } else {
        }

        // if
        *tarr = 105;
        *(tarr + 1) = 102;
        *(tarr + 2) = 0;
        if (strcmp(curStr, tarr) == 0) {
          lchr = toupper(tarr);
          curType = dynarrNewString(tarr, 2) + tarr;
        } else {
        }

        // else
        *tarr = 101;
        *(tarr + 1) = 108;
        *(tarr + 2) = 115;
        *(tarr + 3) = 101;
        *(tarr + 4) = 0;
        if (strcmp(curStr, tarr) == 0) {
          lchr = toupper(tarr);
          curType = dynarrNewString(tarr, 4) + tarr;
        } else {
        }

        // while
        *tarr = 119;
        *(tarr + 1) = 104;
        *(tarr + 2) = 105;
        *(tarr + 3) = 108;
        *(tarr + 4) = 101;
        *(tarr + 5) = 0;
        if (strcmp(curStr, tarr) == 0) {
          lchr = toupper(tarr);
          curType = dynarrNewString(tarr, 5) + tarr;
        } else {
        }

        // return
        *tarr = 114;
        *(tarr + 1) = 101;
        *(tarr + 2) = 116;
        *(tarr + 3) = 117;
        *(tarr + 4) = 114;
        *(tarr + 5) = 110;
        *(tarr + 6) = 0;
        if (strcmp(curStr, tarr) == 0) {
          lchr = toupper(tarr);
          curType = dynarrNewString(tarr, 6) + tarr;
        } else {
        }

        // println
        *tarr = 112;
        *(tarr + 1) = 114;
        *(tarr + 2) = 105;
        *(tarr + 3) = 110;
        *(tarr + 4) = 116;
        *(tarr + 5) = 108;
        *(tarr + 6) = 110;
        *(tarr + 7) = 0;
        if (strcmp(curStr, tarr) == 0) {
          lchr = toupper(tarr);
          curType = dynarrNewString(tarr, 7) + tarr;
        } else {
        }

        if (curType == NULL) {
          *tarr = 73;
          *(tarr + 1) = 68;
          curType = dynarrNewString(tarr, 2) + tarr;
        } else {
        }

        delete[] tarr;
        tokens = tokensPush(stdin, tokens, curStr, curType) + tokens;
      } else {
      }
    } else {
    }
    if (matchingState < 0) {
      matchingState = 0 - matchingState;
    } else {
    }
    if (advance == 1) {
      chr = *stdin;
    } else {
      advance = 1;
    }
  }
  return tokens - stdin;
}

int printTokens(int *stdin, int *stdout, int *tokens) {
  int cnt = 0;
  int size = 0;
  int t = 0;
  int *tarr = NULL;
  int *dynarr = NULL;
  size = dynarrSize(tokens);
  while (cnt < size) {
    tarr = *tokens + stdin;
    dynarr = *tarr + tarr;
    t = print(stdout, dynarr);
    *stdout = 32;
    dynarr = *(tarr + 1) + tarr;
    t = printline(stdout, dynarr);
    cnt = cnt + 1;
    tokens = tokens + 1;
  }
  return 0;
}

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
  delete[] machine;
  return 0;
}
