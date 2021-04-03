#include "9cc.h"

Vector *new_vector() {
  Vector *vec = malloc(sizeof(Vector));
  vec->data = malloc(sizeof(void *) * 16);
  vec->capacity = 16;
  vec->len = 0;
  return vec;
}

void vec_push(Vector *vec, void *elem) {
  if (vec->capacity == vec->len) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
  }
  vec->data[vec->len++] = elem;
}

Vector *vec;

int startswith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

void tokenize(char *p) {
  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
      Token *tok = malloc(sizeof(Token));
      tok->ty = TK_RESERVED;
      tok->input = p;
      tok->len= 2;
      vec_push(vec, tok);
      p += 2;
      continue;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '<' || *p == '>' || *p == ';' || *p == '=') {
      Token *tok = malloc(sizeof(Token));
      tok->ty = TK_RESERVED;
      tok->input = p;
      tok->len = 1;
      vec_push(vec, tok);
      p++;
      continue;
    }

    if ('a' <= *p && *p <= 'z') {
      Token *tok = malloc(sizeof(Token));
      tok->ty = TK_IDENT;
      tok->input = p;
      tok->len = 1;
      vec_push(vec, tok);
      p++;
      continue;
    }

    if (isdigit(*p)) {
      Token *tok = malloc(sizeof(Token));
      tok->ty = TK_NUM;
      tok->input = p;
      char *q = p;
      tok->val = strtol(p, &p, 10);
      tok->len = p - q;
      vec_push(vec, tok);
      continue;
    }

    fprintf(stderr, "Failed to tokenize: %s\n", p);
    exit(1);
  }

  Token *tok = malloc(sizeof(Token));
  tok->ty = TK_EOF;
  tok->input = p;
  tok->len = 0;
  vec_push(vec, tok);
}

void error(char *msg, char *input) {
  fprintf(stderr, msg, input);
  exit(1);
}

int pos = 0;

Node *new_node(int ty, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

Node *new_node_lvar(char *input) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_LVAR;
  node->offset = (input[0] - 'a' + 1) * 8;
  return node;
}

int consume(char *op) {
  Token *tok = vec->data[pos];
  if (tok->ty != TK_RESERVED || strlen(op) != tok->len || memcmp(tok->input, op, tok->len)) {
    return 0;
  }
  pos++;
  return 1;
}

int at_eof() {
  Token *tok = vec->data[pos];
  if (tok->ty == TK_EOF)
    return 1;
  return 0;
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
  Node *node = expr();
  if (!consume(";")) {
    Token *tok = vec->data[pos];
    error("Unterminated statement: %s", tok->input);
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
      Token *tok = vec->data[pos];
      error("Missing closing parenthesis: %s", tok->input);
    }
    return node;
  }

  Token *tok = vec->data[pos];
  if (tok->ty == TK_NUM) {
    pos++;
    return new_node_num(tok->val);
  }
  if (tok->ty == TK_IDENT) {
    pos++;
    return new_node_lvar(tok->input);
  }

  error("Unexpected token found: %s", tok->input);
}
