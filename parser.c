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

char *starts_with_reserved(char *p) {
  static char *kw[] = {"return", "if", "else", "while", "for"};

  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
    int len = strlen(kw[i]);
    if (startswith(p, kw[i]) && !isalnum(p[len]))
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

    if (strchr("+-*/()<>;=", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (isalpha(*p)) {
      char *q = p++;
      while (isalnum(*p))
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

LVar *locals;

LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->input, var->name, var->len))
      return var;
  return NULL;
}

LVar *push_lvar(Token *tok) {
  LVar *var = calloc(1, sizeof(LVar));
  var->next = locals;
  var->name = tok->input;
  var->len = tok->len;
  if (locals) {
    var->offset = locals->offset + 8;
  } else {
    var->offset = 8;
  }
  locals = var;
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

Node *new_node_lvar(LVar *var) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_LVAR;
  node->offset = var->offset;
  return node;
}

int consume(char *op) {
  if (token->ty != TK_RESERVED || strlen(op) != token->len || memcmp(token->input, op, token->len)) {
    return 0;
  }
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

int at_eof() {
  return token->ty == TK_EOF;
}

Node *code[100];

void program() {
  int i = 0;
  while (!at_eof()) {
    code[i++] = stmt();
  }
  code[i] = NULL;
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
      node->init = expr();
      if (!consume(";"))
        error("Unterminated statement: %s", token->input);
    }
    if (!consume(";")) {
      node->cond = expr();
      if (!consume(";"))
        error("Unterminated statement: %s", token->input);
    }
    if (!consume(")")) {
      node->inc = expr();
      if (!consume(")"))
        error("Expected close parenthese: %s", token->input);
    }
    node->then = stmt();
    return node;
  }

  Node *node = expr();
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
    return term();
  if (consume("-"))
    return new_binary_node(ND_SUB, new_node_num(0), term());
  return term();
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
    LVar *var = find_lvar(tok);
    if (!var)
      var = push_lvar(tok);
    return new_node_lvar(var);
  }

  return new_node_num(expect_number());
}
