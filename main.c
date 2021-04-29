#include "9cc.h"

Token *token;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Argument count mismatch\n");
    return 1;
  }

  token = tokenize(argv[1]);
  Function *prog = program();
  add_type(prog);
  codegen(prog);

  return 0;
}
