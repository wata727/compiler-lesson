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

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (startswith(p, "return") && !isalnum(p[6])) {
      cur = new_token(TK_RESERVED, cur, p, 6);
      p += 6;
      continue;
    }

    if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
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

Node *new_node(int ty, Node *lhs, Node *rhs) {
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
  Node *node;
  if (consume("return")) {
    node = new_unary_node(ND_RETURN, expr());
  } else {
    node = expr();
  }

  if (!consume(";")) {
    error("Unterminated statement: %s", token->input);
  }
  return node;
}

Node *expr() {
  return assign();
}

Node *assign() {
  Node *node = equality();
  if (consume("="))
    node = new_node(ND_ASSIGN, node, assign());
  return node;
}

Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("==")) {
      node = new_node(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_node(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<")) {
      node = new_node(ND_LT, node, add());
    } else if (consume("<=")) {
      node = new_node(ND_LE, node, add());
    } else if (consume(">")) {
      node = new_node(ND_LT, add(), node);
    } else if (consume(">=")) {
      node = new_node(ND_LE, add(), node);
    } else {
      return node;
    }
  }
}

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+")) {
      node = new_node(ND_ADD, node, mul());
    } else if (consume("-")) {
      node = new_node(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*")) {
      node = new_node(ND_MUL, node, unary());
    } else if (consume("/")) {
      node = new_node(ND_DIV, node, unary());
    } else {
      return node;
    }
  }
}

Node *unary() {
  if (consume("+"))
    return term();
  if (consume("-"))
    return new_node(ND_SUB, new_node_num(0), term());
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
