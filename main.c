#include "9cc.h"

Token *token;
Node *code[100];
LVar *locals;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Argument count mismatch\n");
    return 1;
  }

  locals = NULL;
  token = tokenize(argv[1]);
  program();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  if (locals)
    printf("  sub rsp, %d\n", locals->offset);

  for (int i = 0; code[i]; i++)
    gen(code[i]);

  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}
