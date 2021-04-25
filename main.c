#include "9cc.h"

Token *token;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Argument count mismatch\n");
    return 1;
  }

  token = tokenize(argv[1]);
  Function *prog = program();

  printf(".intel_syntax noprefix\n");
  for (Function *fn = prog; fn; fn = fn->next) {
    printf(".global %s\n", fn->name);
    printf("%s:\n", fn->name);

    // Prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    if (fn->locals)
      printf("  sub rsp, %d\n", fn->locals->offset);

    // Emit code
    for (Node *n = fn->node; n; n = n->next)
      gen(n);

    // Epilogue
    printf(".Lreturn.%s:\n", fn->name);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }
  return 0;
}
