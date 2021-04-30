#include "9cc.h"

Type *int_type() {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_INT;
  return ty;
}

Type *pointer_to(Type *to) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_PTR;
  ty->ptr_to = to;
  return ty;
}

int size_of(Type *ty) {
  switch (ty->kind) {
  case TY_INT:
    return 4;
  case TY_PTR:
    return 8;
  }
  error("Unexpected type for sizeof", "");
}

void visit(Node *node) {
  if (!node)
    return;

  visit(node->lhs);
  visit(node->rhs);
  visit(node->cond);
  visit(node->then);
  visit(node->els);
  visit(node->init);
  visit(node->inc);

  for (Node *n = node->body; n; n = n->next)
    visit(n);
  for (Node *n = node->args; n; n = n->next)
    visit(n);

  switch (node->ty) {
  case ND_ADD:
  case ND_SUB:
    if (node->rhs->type->kind == TY_PTR)
      error("invalid pointer arithmetic operands", "");
    node->type = node->lhs->type;
    return;
  case ND_MUL:
  case ND_DIV:
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
  case ND_FUNCALL:
  case ND_NUM:
    node->type = int_type();
    return;
  case ND_LVAR:
    node->type = node->var->type;
    return;
  case ND_ASSIGN:
    node->type = node->lhs->type;
    return;
  case ND_ADDR:
    node->type = pointer_to(node->lhs->type);
    return;
  case ND_DEREF:
    if (node->lhs->type->kind != TY_PTR)
      error("invalid pointer dereference", "");
    node->type = node->lhs->type->ptr_to;
    return;
  case ND_SIZEOF:
    node->ty = ND_NUM;
    node->type = int_type();
    node->val = size_of(node->lhs->type);
    node->lhs = NULL;
    return;
  }
}

void add_type(Function *prog) {
  for (Function *fn = prog; fn; fn = fn->next)
    for (Node *node = fn->node; node; node = node->next)
      visit(node);
}
