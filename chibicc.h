#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct Type Type;
typedef struct Node Node;
typedef struct Member Member;

//
// strings.c
//

char *format(char *fmt, ...);

//
// tokenize.c
//

typedef enum {
  TK_IDENT,
  TK_PUNCT,
  TK_KEYWORD,
  TK_STR,
  TK_NUM,
  TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;
  int64_t val;
  char *loc;
  int len;
  Type *ty;
  char *str;
  int line_no;
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...); 
void error_tok(Token *tok, char *fmt, ...); 
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *op); 
bool consume(Token **rest, Token *tok, char *str);
Token *tokenize_file(char *filename);

#define unreachable() \
  error("internal error at %s:%d", __FILE__, __LINE__)

//
// parse.c
//

typedef struct Obj Obj;
struct Obj {
  Obj *next;
  char *name;
  Type *ty;
  bool is_local; // local or global/function

  // Local variable
  int offset;

  // Global variable or function
  bool is_function;
  bool is_definition;
  bool is_static;

  // Global variable
  char *init_data;

  // Function
  Obj *params;
  Node *body;
  Obj *locals;
  int stack_size;
};

typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_NEG,
  ND_MOD,
  ND_BITAND,
  ND_BITOR,
  ND_BITXOR,
  ND_EQ,
  ND_NE,
  ND_LT,
  ND_LE,
  ND_ASSIGN,
  ND_COMMA,
  ND_MEMBER,
  ND_ADDR,
  ND_DEREF,
  ND_NOT,
  ND_BITNOT,
  ND_RETURN,
  ND_IF,
  ND_FOR,
  ND_BLOCK,
  ND_FUNCALL,
  ND_EXPR_STMT,
  ND_STMT_EXPR,
  ND_VAR,
  ND_NUM,
  ND_CAST,
} NodeKind;

struct Node {
  NodeKind kind;
  Node *next;
  Type *ty;
  Token *tok;

  Node *lhs;
  Node *rhs;

  // "if" or "for" statement
  Node *cond;
  Node *then;
  Node *els;
  Node *init;
  Node *inc;

  // Block or statement expression
  Node *body;

  // Struct member access
  Member *member;

  // Function call
  char *funcname;
  Type *func_ty;
  Node *args;

  Obj *var;
  int64_t val;
};

Node *new_cast(Node *expr, Type *ty);
Obj *parse(Token *tok);

//
// type.c
//

typedef enum {
  TY_VOID,
  TY_BOOL,
  TY_CHAR,
  TY_SHORT,
  TY_INT,
  TY_LONG,
  TY_ENUM,
  TY_PTR,
  TY_FUNC,
  TY_ARRAY,
  TY_STRUCT,
  TY_UNION,
} TypeKind;

struct Type {
  TypeKind kind;
  int size;
  int align;

  // Pointer-to or array-of type
  Type *base;

  // Declaration
  Token *name;

  // Array
  int array_len;

  // Struct
  Member *members;

  // Function type
  Type *return_ty;
  Type *params;
  Type *next;
};

// Struct member
struct Member {
  Member *next;
  Type *ty;
  Token *name;
  int offset;
};

extern Type *ty_void;
extern Type *ty_bool;

extern Type *ty_char;
extern Type *ty_short;
extern Type *ty_int;
extern Type *ty_long;

bool is_integer(Type *ty);
Type *copy_type(Type *ty);
Type *pointer_to(Type *base);
Type *func_type(Type *return_ty);
Type *array_of(Type *base, int size);
Type *enum_type(void);
void add_type(Node *node);

//
// codegen.c
//

void codegen(Obj *prog, FILE *out);
int align_to(int n, int align);
