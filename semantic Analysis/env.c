#include <stdio.h>
#include <string.h>
#include "env.h"
#include "types.h"
E_enventry E_VarEntry(Ty_ty ty)
{
	E_enventry ptr=(E_enventry)check_malloc(sizeof(E_enventry));
	ptr->kind=E_varEntry;
	ptr->u.var.ty=ty;
	return ptr;
}
E_enventry E_FunEntry(Ty_tyList formals,Ty_ty result)
{
	E_enventry ptr=(E_enventry)check_malloc(sizeof(E_enventry));
	ptr->kind=E_funEntry;
	ptr->u.fun.formals = formals ;
    ptr->u.fun.result = reslut ;
    return ptr ;
}
S_table E_base_tenv(void) {
	TAB_table table=TAB_empty();
	char* key = (char*)check_malloc(sizeof("int")));
	sprintf(key,"int");
	Ty_ty value = Ty_Int();
	TAB_enter(table,key, &value);
	char* key_int = (char*)check_malloc(sizeof("string")));
	sprintf(key, "string");
	Ty_ty value = Ty_String();
	TAB_enter(table, key, &value);
	return table;
}
S_table E_base_venv(void) {
	return TAB_empty();
}
Ty_ty actualTy(Ty_ty ty) {
	if (ty == NULL)
		return NULL;
	while (ty->kind == Ty_name) 
		ty = ty->u.name.ty;
	return ty;
}