#include "9cc.h"

Token *token;

Token *new_token(int ty, Token *cur, char *input, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->ty = ty;
  tok->input = input;
  tok->len = len;
  cur->next = tok;
  return tok;
}

int startswith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

int is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

int is_alnum(char c) {
  return is_alpha(c) || ('0' <= c && c <= '9');
}

char *starts_with_reserved(char *p) {
  static char *kw[] = {"return", "if", "else", "while", "for", "int", "char", "sizeof"};

  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
    int len = strlen(kw[i]);
    if (startswith(p, kw[i]) && !is_alnum(p[len]))
      return kw[i];
  }

  static char *ops[] = {"==", "!=", "<=", ">="};

  for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++)
    if (startswith(p, ops[i]))
      return ops[i];

  return NULL;
}

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    char *kw = starts_with_reserved(p);
    if (kw) {
      int len = strlen(kw);
      cur = new_token(TK_RESERVED, cur, p, len);
      p += len;
      continue;
    }

    if (strchr("+-*/()<>;={},&[]", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (is_alpha(*p)) {
      char *q = p++;
      while (is_alnum(*p))
        p++;
      cur = new_token(TK_IDENT, cur, q, p - q);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    fprintf(stderr, "Failed to tokenize: %s\n", p);
    exit(1);
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}

void error(char *msg, char *input) {
  fprintf(stderr, msg, input);
  exit(1);
}

VarList *locals;
VarList *globals;

Var *find_var(Token *tok) {
  for (VarList *vl = locals; vl; vl = vl->next) {
    Var *var = vl->var;
    if (var->len == tok->len && !memcmp(tok->input, var->name, var->len))
      return var;
  }
  for (VarList *vl = globals; vl; vl = vl->next) {
    Var *var = vl->var;
    if (var->len == tok->len && !memcmp(tok->input, var->name, var->len))
      return var;
  }
  return NULL;
}

Var *push_var(Token *tok, Type *ty, int is_local) {
  Var *var = calloc(1, sizeof(Var));
  var->name = strndup(tok->input, tok->len);
  var->len = tok->len;
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

int peek(char *op) {
  if (token->ty != TK_RESERVED || strlen(op) != token->len || memcmp(token->input, op, token->len))
    return 0;
  return 1;
}

int consume(char *op) {
  if (!peek(op))
    return 0;
  token = token->next;
  return 1;
}

Token *consume_ident() {
  if (token->ty != TK_IDENT)
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

int expect_number() {
  if (token->ty != TK_NUM)
    error("expect a number: %s", token->input);
  int val = token->val;
  token = token->next;
  return val;
}

Token *expect_ident() {
  if (token->ty != TK_IDENT)
    error("expect an identifier: %s", token->input);
  Token *t = token;
  token = token->next;
  return t;
}

int at_eof() {
  return token->ty == TK_EOF;
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
      error("expected type: %s", token->input);
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
    error("number is expected for array size", "");
  base = read_type_suffix(base);
  return array_of(base, sz);
}

VarList *read_func_param() {
  Type *ty = basetype();
  Token *tok = expect_ident();
  ty = read_type_suffix(ty);

  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = push_var(tok, ty, 1);
  return vl;
}

VarList *read_func_params() {
  if (consume(")"))
    return NULL;

  VarList *head = read_func_param();
  VarList *cur = head;

  while (!consume(")")) {
    if (!consume(","))
      error("Function arguments must be split by comma: %s", token->input);
    cur->next = read_func_param();
    cur = cur->next;
  }

  return head;
}

Function *function() {
  locals = NULL;

  Function *fn = calloc(1, sizeof(Function));
  basetype();
  Token *t = expect_ident();
  fn->name = strndup(t->input, t->len);
  if (!consume("("))
    error("Function identifier must be followed by open parenthese: %s", token->input);
  fn->params = read_func_params();
  if (!consume("{"))
    error("Function must start from block: %s", token->input);

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
  Token *tok = expect_ident();
  ty = read_type_suffix(ty);
  if (!consume(";"))
    error("Unterminated global variable declaration: %s", tok->input);
  push_var(tok, ty, 0);
}

Node *declaration() {
  Type *ty = basetype();
  Token *tok = expect_ident();
  ty = read_type_suffix(ty);
  push_var(tok, ty, 1);

  if (consume(";"))
    return new_node(ND_NULL);
  error("Unterminated declaration: %s", token->input);
}

int is_typename() {
  return peek("int") || peek("char");
}

Node *stmt() {
  if (consume("return")) {
    Node *node = new_unary_node(ND_RETURN, expr());
    if (!consume(";"))
      error("Unterminated statement: %s", token->input);
    return node;
  }

  if (consume("if")) {
    Node *node = new_node(ND_IF);
    if (!consume("("))
      error("Expected open parenthese: %s", token->input);
    node->cond = expr();
    if (!consume(")"))
      error("Expected close parenthese: %s", token->input);
    node->then = stmt();
    if (consume("else"))
      node->els = stmt();
    return node;
  }

  if (consume("while")) {
    Node *node = new_node(ND_WHILE);
    if (!consume("("))
      error("Expected open parenthese: %s", token->input);
    node->cond = expr();
    if (!consume(")"))
      error("Expected close parenthese: %s", token->input);
    node->then = stmt();
    return node;
  }

  if (consume("for")) {
    Node *node = new_node(ND_FOR);
    if (!consume("("))
      error("Expected open parenthese: %s", token->input);
    if (!consume(";")) {
      node->init = read_expr_stmt();
      if (!consume(";"))
        error("Unterminated statement: %s", token->input);
    }
    if (!consume(";")) {
      node->cond = expr();
      if (!consume(";"))
        error("Unterminated statement: %s", token->input);
    }
    if (!consume(")")) {
      node->inc = read_expr_stmt();
      if (!consume(")"))
        error("Expected close parenthese: %s", token->input);
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
    error("Unterminated statement: %s", token->input);
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
      error("Missing closing bracket: %s", token->input);
    node = new_unary_node(ND_DEREF, exp);
  }
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
    error("Missing closing parenthesis: %s", token->input);
  }
  return head;
}

Node *term() {
  if (consume("(")) {
    Node *node = add();
    if (!consume(")")) {
      error("Missing closing parenthesis: %s", token->input);
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
      error("Undefined variable: %s", tok->input);
    return new_node_var(var);
  }

  return new_node_num(expect_number());
}
