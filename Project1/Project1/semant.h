#include "types.h"
#include "env.h"
#include "absyn.h"
#include "errormsg.h"
struct expty { Tr_exp exp; Ty_ty ty; };
struct expty transVar(S_table venv,S_table tenv,A_var v, Tr_level level,Temp_label done);
struct expty transExp(S_table venv,S_table tenv,A_exp v, Tr_level level, Temp_label done);
Tr_exp transDec(S_table venv,S_table tenv,A_dec v,Tr_level level, Temp_label done);
Ty_ty transTy(S_table tenv,A_ty v);
