#include "9cc.h"

int labelseq = 0;
char *argreg1[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
char *argreg8[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

void codegen(Program *prog) {
  printf(".intel_syntax noprefix\n");

  // data
  printf(".data\n");

  for (VarList *vl = prog->globals; vl; vl = vl->next) {
    Var *var = vl->var;
    printf("%s:\n", var->name);
    printf("  .zero %d\n", size_of(var->type));
  }

  // text
  printf(".text\n");

  for (Function *fn = prog->fns; fn; fn = fn->next) {
    printf(".global %s\n", fn->name);
    printf("%s:\n", fn->name);

    // Prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    if (fn->locals)
      printf("  sub rsp, %d\n", fn->stack_size);

    int i = 0;
    for (VarList *vl = fn->params; vl; vl = vl->next) {
      Var *var = vl->var;
      int sz = size_of(var->type);
      if (sz == 1) {
        printf("  mov [rbp-%d], %s\n", var->offset, argreg1[i++]);
      } else if (sz == 8) {
        printf("  mov [rbp-%d], %s\n", var->offset, argreg8[i++]);
      } else {
        error("Unexpected variable size", "");
      }
    }

    // Emit code
    for (Node *n = fn->node; n; n = n->next)
      gen(n);

    // Epilogue
    printf(".Lreturn.%s:\n", fn->name);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }
}

void gen_lval(Node *node) {
  switch (node->ty) {
  case ND_VAR:
    if (node->var->is_local) {
      printf("  mov rax, rbp\n");
      printf("  sub rax, %d\n", node->var->offset);
      printf("  push rax\n");
    } else {
      printf("  push offset %s\n", node->var->name);
    }
    return;
  case ND_DEREF:
    gen(node->lhs);
    return;
  }

  error("left value is not variable or dereference", "");
}

void load(Type *ty) {
  printf("  pop rax\n");
  if (size_of(ty) == 1)
    printf("  movsx rax, byte ptr [rax]\n");
  else
    printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

void store(Type *ty) {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  if (size_of(ty) == 1)
    printf("  mov [rax], dil\n");
  else
    printf("  mov [rax], rdi\n");
  printf("  push rdi\n");
}

void gen(Node *node) {
  int seq;

  switch (node->ty) {
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_VAR:
      gen_lval(node);
      if (node->type->kind != TY_ARRAY)
        load(node->type);
      return;
    case ND_ASSIGN:
      if (node->lhs->type->kind == TY_ARRAY)
        error("not an lvalue", "");
      gen_lval(node->lhs);
      gen(node->rhs);
      store(node->type);
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
    case ND_FUNCALL: {
      int nargs = 0;
      for (Node *arg = node->args; arg; arg = arg->next) {
        gen(arg);
	nargs++;
      }

      for (int i = nargs -1; i >= 0; i--)
        printf("  pop %s\n", argreg8[i]);

      int seq = labelseq++;
      printf("  mov rax, rsp\n");
      printf("  and rax, 15\n");
      printf("  jnz .Lcall%d\n", seq);
      printf("  mov rax, 0\n");
      printf("  call %s\n", node->funcname);
      printf("  jmp .Lend%d\n", seq);
      printf(".Lcall%d:\n", seq);
      printf("  sub rsp, 8\n");
      printf("  mov rax, 0\n");
      printf("  call %s\n", node->funcname);
      printf("  add rsp, 8\n");
      printf(".Lend%d:\n", seq);
      printf("  push rax\n");
      return;
    }
    case ND_ADDR:
      gen_lval(node->lhs);
      return;
    case ND_DEREF:
      gen(node->lhs);
      if (node->type->kind != TY_ARRAY)
        load(node->type);
      return;
    case ND_NULL:
      return;
  }
  

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->ty) {
    case ND_ADD:
      if (node->type->ptr_to)
        printf("  imul rdi, %d\n", size_of(node->type->ptr_to));
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      if (node->type->ptr_to)
        printf("  imul rdi, %d\n", size_of(node->type->ptr_to));
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
