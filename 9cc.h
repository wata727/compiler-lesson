#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

enum {
  TK_RESERVED,
  TK_IDENT,
  TK_NUM,
  TK_EOF,
};

typedef struct Token Token;
struct Token {
  int ty;
  Token *next;
  int val;
  char *input;
  int len;
};

Token *new_token(int ty, Token *cur, char *input, int len);
int startswith(char *p, char *q);
Token *tokenize(char *p);
void error(char *msg, char *input);

typedef struct LVar LVar;
struct LVar {
  LVar *next;
  char *name;
  int len;
  int offset;
};

enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_EQ,
  ND_NE,
  ND_LT,
  ND_LE,
  ND_ASSIGN,
  ND_LVAR,
  ND_NUM,
};

typedef struct Node {
  int ty;
  struct Node *lhs;
  struct Node *rhs;
  int val;
  int offset;
} Node;

LVar *find_lvar(Token *tok);
LVar *push_lvar(Token *tok);
Node *new_node(int ty, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_node_lvar(LVar *var);
int consume(char *op);
Token *consume_ident();
int expect_number();
int at_eof();
void program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *term();

void gen(Node *node);

int expect(int line, int expected, int acutual);
void runtest();
