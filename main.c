#include "9cc.h"

Token *token;

int align_to(int n, int align) {
  return (n + align - 1) & ~(align - 1);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Argument count mismatch\n");
    return 1;
  }

  token = tokenize(argv[1]);
  Program *prog = program();
  add_type(prog);

  for (Function *fn = prog->fns; fn; fn = fn->next) {
    int offset = 0;
    for (VarList *vl = fn->locals; vl; vl = vl->next) {
      Var *var = vl->var;
      offset += size_of(var->type);
      var->offset = offset;
    }
    fn->stack_size = align_to(offset, 8);
  }
  codegen(prog);

  return 0;
}
