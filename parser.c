#include "9cc.h"

VarList *locals;
VarList *globals;

Var *find_var(Token *tok) {
  for (VarList *vl = locals; vl; vl = vl->next) {
    Var *var = vl->var;
    if (strlen(var->name) == tok->len && !memcmp(tok->input, var->name, tok->len))
      return var;
  }
  for (VarList *vl = globals; vl; vl = vl->next) {
    Var *var = vl->var;
    if (strlen(var->name) == tok->len && !memcmp(tok->input, var->name, tok->len))
      return var;
  }
  return NULL;
}

Var *push_var(char *name, Type *ty, int is_local) {
  Var *var = calloc(1, sizeof(Var));
  var->name = name;
  var->type = ty;
  var->is_local = is_local;

  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = var;
  if (is_local) {
    vl->next = locals;
    locals = vl;
  } else {
    vl->next = globals;
    globals = vl;
  }
  return var;
}

char *new_label() {
  static int cnt = 0;
  char buf[20];
  sprintf(buf, ".L.data.%d", cnt++);
  return strndup(buf, 20);
}

Node *new_node(int ty) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  return node;
}

Node *new_binary_node(int ty, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_unary_node(int ty, Node *lhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

Node *new_node_var(Var *var) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_VAR;
  node->var = var;
  return node;
}

int is_function() {
  Token *tok = token;
  basetype();
  int isfunc = consume_ident() && consume("(");
  token = tok;
  return isfunc;
}

Program *program() {
  Function head;
  head.next = NULL;
  Function *cur = &head;
  globals = NULL;

  while (!at_eof()) {
    if (is_function()) {
      cur->next = function();
      cur = cur->next;
    } else {
      global_var();
    }
  }

  Program *prog = calloc(1, sizeof(Program));
  prog->globals = globals;
  prog->fns = head.next;
  return prog;
}

Node *read_expr_stmt() {
  return new_unary_node(ND_EXPR_STMT, expr());
}

Type *basetype() {
  Type *ty;
  if (consume("char")) {
    ty = char_type();
  } else {
    if (!consume("int"))
      error_at("expected type", token->input);
    ty = int_type();
  }

  while(consume("*"))
    ty = pointer_to(ty);
  return ty;
}

Type *read_type_suffix(Type *base) {
  if (!consume("["))
    return base;
  int sz = expect_number();
  if (!consume("]"))
    error_at("number is expected for array size", token->input);
  base = read_type_suffix(base);
  return array_of(base, sz);
}

VarList *read_func_param() {
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);

  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = push_var(name, ty, 1);
  return vl;
}

VarList *read_func_params() {
  if (consume(")"))
    return NULL;

  VarList *head = read_func_param();
  VarList *cur = head;

  while (!consume(")")) {
    if (!consume(","))
      error_at("Function arguments must be split by comma", token->input);
    cur->next = read_func_param();
    cur = cur->next;
  }

  return head;
}

Function *function() {
  locals = NULL;

  Function *fn = calloc(1, sizeof(Function));
  basetype();
  char *name = expect_ident();
  fn->name = name;
  if (!consume("("))
    error_at("Function identifier must be followed by open parenthese", token->input);
  fn->params = read_func_params();
  if (!consume("{"))
    error_at("Function must start from block", token->input);

  Node head;
  head.next = NULL;
  Node *cur = &head;

  while (!consume("}")) {
    cur->next = stmt();
    cur = cur->next;
  }

  fn->node = head.next;
  fn->locals = locals;
  return fn;
}

void global_var() {
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);
  if (!consume(";"))
    error_at("Unterminated global variable declaration", token->input);
  push_var(name, ty, 0);
}

Node *declaration() {
  Type *ty = basetype();
  char *name = expect_ident();
  ty = read_type_suffix(ty);
  push_var(name, ty, 1);

  if (consume(";"))
    return new_node(ND_NULL);
  error_at("Unterminated declaration", token->input);
}

int is_typename() {
  return peek("int") || peek("char");
}

Node *stmt() {
  if (consume("return")) {
    Node *node = new_unary_node(ND_RETURN, expr());
    if (!consume(";"))
      error_at("Unterminated statement", token->input);
    return node;
  }

  if (consume("if")) {
    Node *node = new_node(ND_IF);
    if (!consume("("))
      error_at("Expected open parenthese", token->input);
    node->cond = expr();
    if (!consume(")"))
      error_at("Expected close parenthese", token->input);
    node->then = stmt();
    if (consume("else"))
      node->els = stmt();
    return node;
  }

  if (consume("while")) {
    Node *node = new_node(ND_WHILE);
    if (!consume("("))
      error_at("Expected open parenthese", token->input);
    node->cond = expr();
    if (!consume(")"))
      error_at("Expected close parenthese", token->input);
    node->then = stmt();
    return node;
  }

  if (consume("for")) {
    Node *node = new_node(ND_FOR);
    if (!consume("("))
      error_at("Expected open parenthese", token->input);
    if (!consume(";")) {
      node->init = read_expr_stmt();
      if (!consume(";"))
        error_at("Unterminated statement", token->input);
    }
    if (!consume(";")) {
      node->cond = expr();
      if (!consume(";"))
        error_at("Unterminated statement", token->input);
    }
    if (!consume(")")) {
      node->inc = read_expr_stmt();
      if (!consume(")"))
        error_at("Expected close parenthese", token->input);
    }
    node->then = stmt();
    return node;
  }

  if (consume("{")) {
    Node head;
    head.next = NULL;
    Node *cur = &head;

    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }

    Node *node = new_node(ND_BLOCK);
    node->body = head.next;
    return node;
  }

  if (is_typename())
    return declaration();

  Node *node = read_expr_stmt();
  if (!consume(";")) 
    error_at("Unterminated statement", token->input);
  return node;
}

Node *expr() {
  return assign();
}

Node *assign() {
  Node *node = equality();
  if (consume("="))
    node = new_binary_node(ND_ASSIGN, node, assign());
  return node;
}

Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("==")) {
      node = new_binary_node(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_binary_node(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<")) {
      node = new_binary_node(ND_LT, node, add());
    } else if (consume("<=")) {
      node = new_binary_node(ND_LE, node, add());
    } else if (consume(">")) {
      node = new_binary_node(ND_LT, add(), node);
    } else if (consume(">=")) {
      node = new_binary_node(ND_LE, add(), node);
    } else {
      return node;
    }
  }
}

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+")) {
      node = new_binary_node(ND_ADD, node, mul());
    } else if (consume("-")) {
      node = new_binary_node(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*")) {
      node = new_binary_node(ND_MUL, node, unary());
    } else if (consume("/")) {
      node = new_binary_node(ND_DIV, node, unary());
    } else {
      return node;
    }
  }
}

Node *unary() {
  if (consume("+"))
    return unary();
  if (consume("-"))
    return new_binary_node(ND_SUB, new_node_num(0), unary());
  if (consume("&"))
    return new_unary_node(ND_ADDR, unary());
  if (consume("*"))
    return new_unary_node(ND_DEREF, unary());
  if (consume("sizeof"))
    return new_unary_node(ND_SIZEOF, unary());
  return postfix();
}

Node *postfix() {
  Node *node = term();

  while (consume("[")) {
    Node *exp = new_binary_node(ND_ADD, node, expr());
    if (!consume("]"))
      error_at("Missing closing bracket", token->input);
    node = new_unary_node(ND_DEREF, exp);
  }
  return node;
}

Node *stmt_expr() {
  Node *node = new_node(ND_STMT_EXPR);
  node->body = stmt();
  Node *cur = node->body;

  while (!consume("}")) {
    cur->next = stmt();
    cur = cur->next;
  }
  if (!consume(")"))
    error_at("Missing closing parenthesis", token->input);

  if (cur->ty != ND_EXPR_STMT)
    error_at("stmt expr returning void is not supported", token->input);
  *cur = *cur->lhs;
  return node;
}

Node *func_args() {
  if (consume(")"))
    return NULL;

  Node *head = assign();
  Node *cur = head;
  while (consume(",")) {
    cur->next = assign();
    cur = cur->next;
  }
  if (!consume(")")) {
    error_at("Missing closing parenthesis", token->input);
  }
  return head;
}

Node *term() {
  if (consume("(")) {
    if (consume("{"))
      return stmt_expr();

    Node *node = add();
    if (!consume(")")) {
      error_at("Missing closing parenthesis", token->input);
    }
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    if (consume("(")) {
      Node *node = new_node(ND_FUNCALL);
      node->funcname = strndup(tok->input, tok->len);
      node->args = func_args();
      return node;
    }

    Var *var = find_var(tok);
    if (!var)
      error_at("Undefined variable", tok->input);
    return new_node_var(var);
  }

  tok = token;
  if (tok->ty == TK_STR) {
    token = token->next;

    Type *ty = array_of(char_type(), tok->cont_len);
    Var *var = push_var(new_label(), ty, 0);
    var->contents = tok->contents;
    var->cont_len = tok->cont_len;
    return new_node_var(var);
  }

  return new_node_num(expect_number());
}
