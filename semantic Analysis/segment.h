#include "types.h"
#include "env.h"
#include "absyn.h"
struct expty{Tr_exp exp; Ty_ty ty;};
struct expty transVar(S_table venv,S_table tenv,A_var v);
struct expty transExp(S_table venv,S_table tenv,A_exp v);
void transDec(S_table venv,S_table tenv,A_dec v);
Ty_ty transTy(S_table tenv,A_ty v);
