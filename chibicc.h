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

#define MAX(x, y) ((x) < (y) ? (y) : (x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

typedef struct Type Type;
typedef struct Node Node;
typedef struct Member Member;
typedef struct Relocation Relocation;

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
  Token *tok;
  bool is_local; // local or global/function
  int align;

  // Local variable
  int offset;

  // Global variable or function
  bool is_function;
  bool is_definition;
  bool is_static;

  // Global variable
  char *init_data;
  Relocation *rel;

  // Function
  Obj *params;
  Node *body;
  Obj *locals;
  Obj *va_area;
  int stack_size;
};

typedef struct Relocation Relocation;
struct Relocation {
  Relocation *next;
  int offset;
  char *label;
  long addend;
};

typedef enum {
  ND_NULL_EXPR,
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_NEG,
  ND_MOD,
  ND_BITAND,
  ND_BITOR,
  ND_BITXOR,
  ND_SHL,
  ND_SHR,
  ND_EQ,
  ND_NE,
  ND_LT,
  ND_LE,
  ND_ASSIGN,
  ND_COND,
  ND_COMMA,
  ND_MEMBER,
  ND_ADDR,
  ND_DEREF,
  ND_NOT,
  ND_BITNOT,
  ND_LOGAND,
  ND_LOGOR,
  ND_RETURN,
  ND_IF,
  ND_FOR,
  ND_DO,
  ND_SWITCH,
  ND_CASE,
  ND_BLOCK,
  ND_GOTO,
  ND_LABEL,
  ND_FUNCALL,
  ND_EXPR_STMT,
  ND_STMT_EXPR,
  ND_VAR,
  ND_NUM,
  ND_CAST,
  ND_MEMZERO,
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

  // "break" and "continue" labels
  char *brk_label;
  char *cont_label;

  // Goto or labeled statement
  char *label;
  char *unique_label;
  Node *goto_next;

  // Swicth-cases
  Node *case_next;
  Node *default_case;

  // Variable
  Obj *var;

  // Numeric literal
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
  bool is_unsigned;

  // Pointer-to or array-of type
  Type *base;

  // Declaration
  Token *name;
  Token *name_pos;

  // Array
  int array_len;

  // Struct
  Member *members;
  bool is_flexible;

  // Function type
  Type *return_ty;
  Type *params;
  bool is_variadic;
  Type *next;
};

// Struct member
struct Member {
  Member *next;
  Type *ty;
  Token *tok;
  Token *name;
  int idx;
  int align;
  int offset;
};

extern Type *ty_void;
extern Type *ty_bool;

extern Type *ty_char;
extern Type *ty_short;
extern Type *ty_int;
extern Type *ty_long;

extern Type *ty_uchar;
extern Type *ty_ushort;
extern Type *ty_uint;
extern Type *ty_ulong;

bool is_integer(Type *ty);
Type *copy_type(Type *ty);
Type *pointer_to(Type *base);
Type *func_type(Type *return_ty);
Type *array_of(Type *base, int size);
Type *enum_type(void);
Type *struct_type(void);
void add_type(Node *node);

//
// codegen.c
//

void codegen(Obj *prog, FILE *out);
int align_to(int n, int align);
