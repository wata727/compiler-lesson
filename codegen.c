#include "9cc.h"

int labelseq = 0;

void gen_lval(Node *node) {
  if (node->ty != ND_LVAR)
    error("left value is not variable", "");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen(Node *node) {
  int seq;

  switch (node->ty) {
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_LVAR:
      gen_lval(node);
      printf("  pop rax\n");
      printf("  mov rax, [rax]\n");
      printf("  push rax\n");
      return;
    case ND_ASSIGN:
      gen_lval(node->lhs);
      gen(node->rhs);

      printf("  pop rdi\n");
      printf("  pop rax\n");
      printf("  mov [rax], rdi\n");
      printf("  push rdi\n");
      return;
    case ND_RETURN:
      gen(node->lhs);
      printf("  pop rax\n");
      printf("  mov rsp, rbp\n");
      printf("  pop rbp\n");
      printf("  ret\n");
      return;
    case ND_EXPR_STMT:
      gen(node->lhs);
      printf("  add rsp, 8\n");
      return;
    case ND_IF:
      seq = labelseq++;
      if (node->els) {
        gen(node->cond);
	printf("  pop rax\n");
	printf("  cmp rax, 0\n");
	printf("  je .Lelse%d\n", seq);
	gen(node->then);
	printf("  jmp .Lend%d\n", seq);
	printf(".Lelse%d:\n", seq);
	gen(node->els);
	printf(".Lend%d:\n", seq);
      } else {
        gen(node->cond);
	printf("  pop rax\n");
	printf("  cmp rax, 0\n");
	printf("  je .Lend%d\n", seq);
	gen(node->then);
	printf(".Lend%d:\n", seq);
      }
      return;
    case ND_WHILE:
      seq = labelseq++;
      printf(".Lbegin%d:\n", seq);
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .Lend%d\n", seq);
      gen(node->then);
      printf("  jmp .Lbegin%d\n", seq);
      printf(".Lend%d:\n", seq);
      return;
    case ND_FOR:
      seq = labelseq++;
      if (node->init)
        gen(node->init);
      printf(".Lbegin%d:\n", seq);
      if (node->cond) {
        gen(node->cond);
	printf("  pop rax\n");
	printf("  cmp rax, 0\n");
	printf("  je .Lend%d\n", seq);
      }
      gen(node->then);
      if (node->inc)
        gen(node->inc);
      printf("  jmp .Lbegin%d\n", seq);
      printf(".Lend%d:\n", seq);
      return;
    case ND_BLOCK:
      for (Node *n = node->body; n; n = n->next)
        gen(n);
      return;
  }
  

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->ty) {
    case ND_ADD:
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  mul rdi\n");
      break;
    case ND_DIV:
      printf("  mov rdx, 0\n");
      printf("  div rdi\n");
      break;
    case ND_EQ:
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_NE:
      printf("  cmp rax, rdi\n");
      printf("  setne al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LT:
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LE:
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzb rax, al\n");
      break;
  }

  printf("  push rax\n");
}
