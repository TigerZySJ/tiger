typedef struct E_enventry_ *E_enventry;
struct E_enventry_ {enum {E_varEntry,E_funEntry} kind;
	union {struct {Ty_ty ty;}var;
			struct {Ty_tyList formals;Ty_ty result}fun;}u;
			};
	}
E_enventry E_VarEntry(Ty_ty ty);
E_enventry E_FunEntry(Ty_tyList formals,Ty_ty result);
//initalize type environment
//in tiger we need initialize int/string type
S_table E_base_tenv(void);
//initalize value environment
//in tiger we don't need initilize any value; 
S_table E_base_venv(void);
//get actual type of a variable
Ty_ty actualTy(Ty_ty ty);