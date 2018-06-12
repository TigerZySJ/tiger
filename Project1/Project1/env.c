#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "env.h"
#include "types.h"
E_enventry E_VarEntry(Tr_access access,Ty_ty ty)
{
	E_enventry ptr=(E_enventry)checked_malloc(sizeof(*ptr));
	ptr->kind=E_varEntry;
	ptr->u.var.access = access;
	ptr->u.var.ty=ty;
	return ptr;
}
E_enventry E_FunEntry(Tr_level level, Temp_label label,Ty_tyList formals,Ty_ty result)
{
	E_enventry ptr=(E_enventry)checked_malloc(sizeof(*ptr));
	ptr->kind=E_funEntry;
	ptr->u.fun.level = level;
	ptr->u.fun.label = label;
	ptr->u.fun.formals = formals ;
	ptr->u.fun.result = result;
    return ptr ;
}
S_table E_base_tenv(void) {
	TAB_table table=TAB_empty();
	Ty_ty value = Ty_Int();
	TAB_enter(table, S_Symbol("int"), value);
	value = Ty_String();
	TAB_enter(table, S_Symbol("string"), value);
	value = Ty_Nil();
	TAB_enter(table, S_Symbol("nil"), value);
	return table;
}
S_table E_base_venv(void) {
	return TAB_empty();
}
Ty_ty actualTy(Ty_ty ty) {
	if (ty == NULL)
		return Ty_Void();
	while (ty->kind == Ty_name) 
		ty = ty->u.name.ty;
	return ty;
}
bool TyEqual(Ty_ty ty1, Ty_ty ty2) {
	ty1 = actualTy(ty1);
	ty2 = actualTy(ty2);
	bool isNil = (ty1->kind == Ty_nil || ty2->kind == Ty_nil);
	bool isRecord = (ty1->kind == Ty_record || ty2->kind == Ty_record);
	if (ty1->kind != ty2->kind) {
		if (isNil&&isRecord)
			return true;
		return false;
	}
	return ty1==ty2;
}