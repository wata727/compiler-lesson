#include "9cc.h"

int expect(int line, int expected, int acutual) {
  if (expected == acutual) {
    return 0;
  }
  fprintf(stderr, "%d: %d expected, but got %d\n", line, expected, acutual);
  exit(1);
}

void runtest() {
  Vector *vec = new_vector();
  expect(__LINE__, 0, vec->len);

  for (int i = 0; i < 100; i++) {
    vec_push(vec, (void *)(intptr_t)i);
  }

  expect(__LINE__, 100, vec->len);
  expect(__LINE__, 0, (intptr_t)vec->data[0]);
  expect(__LINE__, 50, (intptr_t)vec->data[50]);
  expect(__LINE__, 99, (intptr_t)vec->data[99]);

  printf("OK\n");
}

Vector *vec;
Node *code[100];

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Argument count mismatch\n");
    return 1;
  }
  if (strcmp(argv[1], "-test") == 0) {
    runtest();
    return 0;
  }

  vec = new_vector();

  tokenize(argv[1]);
  program();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 208\n");

  for (int i = 0; code[i]; i++) {
    gen(code[i]);

    printf("  pop rax\n");
  }

  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}
