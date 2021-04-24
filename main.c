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
  printf(".global main\n");
  printf("main:\n");

  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  if (prog->locals)
    printf("  sub rsp, %d\n", prog->locals->offset);

  for (Node *n = prog->node; n; n = n->next)
    gen(n);

  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}
