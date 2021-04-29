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
char *starts_with_reserved(char *p);
Token *tokenize(char *p);
void error(char *msg, char *input);

typedef enum { TY_INT, TY_PTR } TypeKind;
typedef struct Type Type;
struct Type {
  TypeKind kind;
  Type *ptr_to;
};

typedef struct LVar LVar;
struct LVar {
  char *name;
  int len;
  Type *type;
  int offset;
};

typedef struct VarList VarList;
struct VarList {
  struct VarList *next;
  LVar *var;
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
  ND_RETURN,
  ND_EXPR_STMT,
  ND_IF,
  ND_WHILE,
  ND_FOR,
  ND_BLOCK,
  ND_FUNCALL,
  ND_ADDR,
  ND_DEREF,
  ND_NULL,
};

typedef struct Node {
  int ty;
  struct Node *next;
  Type *type;

  struct Node *lhs;
  struct Node *rhs;

  struct Node *cond;
  struct Node *then;
  struct Node *els;
  struct Node *init;
  struct Node *inc;
  struct Node *body;
  struct Node *args;

  int val;
  LVar *var;
  char *funcname;
} Node;

typedef struct Function {
  struct Function *next;
  char *name;
  VarList *params;

  Node *node;
  VarList *locals;
} Function;

LVar *find_lvar(Token *tok);
LVar *push_lvar(Token *tok, Type *ty);
Node *new_node(int ty);
Node *new_binary_node(int ty, Node *lhs, Node *rhs);
Node *new_unary_node(int ty, Node *lhs);
Node *new_node_num(int val);
Node *new_node_lvar(LVar *var);
int peek(char *op);
int consume(char *op);
Token *consume_ident();
int expect_number();
Token *expect_ident();
int at_eof();
Function *program();
Function *function();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *func_args();
Node *term();

void codegen(Function *prog);
void gen(Node *node);

Type *int_type();
Type *pointer_to(Type *to);
void add_type(Function *prog);

int expect(int line, int expected, int acutual);
void runtest();
