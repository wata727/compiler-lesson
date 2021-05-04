#include "9cc.h"

char *filename;
char *user_input;
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
    error_at("expect a number", token->input);
  int val = token->val;
  token = token->next;
  return val;
}

char *expect_ident() {
  if (token->ty != TK_IDENT)
    error_at("expect an identifier", token->input);
  char *s = strndup(token->input, token->len);
  token = token->next;
  return s;
}

int at_eof() {
  return token->ty == TK_EOF;
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

    if (strncmp(p, "//", 2) == 0) {
      p += 2;
      while (*p != '\n')
        p++;
      continue;
    }

    if (strncmp(p, "/*", 2) == 0) {
      char *q = strstr(p + 2, "*/");
      if (!q)
        error_at("Missing closing comment", cur->input);
      p = q + 2;
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

    if (*p == '"') {
      char *q = p++;
      while (*p && *p != '"')
        p++;
      if (!*p)
        error_at("Missing closing double quote", cur->input);
      p++;

      cur = new_token(TK_STR, cur, q, p - q);
      cur->contents = strndup(q + 1, p - q -2);
      cur->cont_len = p - q- 1;
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

void error_at(char *msg, char *loc) {
  char *line = loc;
  while (user_input < line && line[-1] != '\n')
    line--;

  char *end = loc;
  while (*end != '\n')
    end++;

  int line_num = 1;
  for (char *p = user_input; p < line; p++)
    if (*p == '\n')
      line_num++;

  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  int pos = loc - line + indent;
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ %s\n", msg);
  exit(1);
}
