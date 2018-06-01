#include <stdio.h>
#include <string.h>
#include "segment.h"
//IR,we temporarily define it as void*
typedef void* Tr_exp;
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
    ptr->u.fun.result = result ;
    return ptr ;
}
struct expty expTy(Tr_exp exp, Ty_ty ty){
	struct expty e;
	e.exp=exp;
	e.ty=ty;
	return e;
}
struct expty transVar(S_table venv,S_table tenv,A_var v)
{
	struct expty tmp_expty;
	switch(v->kind){
		case A_simpleVar:{
			E_enventry x=S_look(venv,v->u.simple);
			if(!x||x->kind!=E_varEntry)
			{
				EM_error(v->pos,"undefined variable %s",
					S_name(v->u.simple));
				assert(0);
			}
			return expTy(NULL, actual_ty(x->u.var.ty));
		}
		case A_fieldVar:{
			//遇到A.b这种形式我们需要先检查A是否合乎语义
			tmp_expty = transVar(venv, tenv, v->u.field.var);
			//A的类型必定是ty_record，即必须是fieldlist,否则报错
			if (tmp_expty.ty != Ty_record) {
				EM_error(v->pos, "undefined FieldList");
				assert(0);
			}
			//如果是field list需要遍历fieldlist寻找到该值
			Ty_fieldList record = tmp_expty.ty->u.record;
			if (record == NULL) {
				EM_error(v->pos, "undefined FieldList");
				assert(0);
			}
			while (record->head->name != v->u.field.sym) {
				record = record->tail;
				if (record == NULL) {
					EM_error(v->pos, "undefined FieldList");
					assert(0);
				}
			}
			return expty(NULL, actualTy(record->head->ty));
		}
		case A_subscriptVar:{
			
			tmp_expty =transVar(venv,tenv,v->u.subscript.var);
			//如果不是数组，则报错
			if(tmp_expty.ty!=Ty_Array){
				EM_error(v->pos,"undefined array");
				assert(0);
			}
			tmp_expty =transExp(venv,tenv,v->u.subscript.exp);
			//如果数组内返回值不是int
			if(tmp_expty.ty!=Ty_int){
				EM_error(v->pos,"The index of array is not int");
				assert(0);
			}
			return tmp_expty;
		}
	}
}
struct expty transExp(S_table venv,S_table tenv,A_exp v)
{
	struct expty tmp_expty;
	switch(a->kind){
		case A_opExp:{
			A_oper oper=a->u.op.oper;
			struct expty left=transExp(venv,tenv,a->u.op.left);
			struct expty right=transExp(venv,tenv,a->u.op.right);
			switch (oper) {
				case A_plusOp:
				case A_minusOp:
				case A_timesOp:
				case A_divideOp：
				case A_ltOp:
				case A_leOp:
				case A_gtOp:
				case A_geOp: {
					//上述操作符都需要整数运算
					if (left.ty->kind != Ty_int) {
						EM_error(a->u.op.left->pos, "integer required");
						assert(0);
					}
					if (right.ty->kind != Ty_int) {
						EM_error(a->u.op.right->pos, "integer required");
						assert(0);
					}
					return expTy(NULL, Ty_Int());
				}
				case A_eqOp:
				case A_neqOp: {					
					if(left.ty->kind==Ty_int&&left.ty->right == Ty_int)
						return expTy(NULL, Ty_Int());
					if (left.ty->kind == Ty_string&&left.ty->right == Ty_string)
						return expTy(NULL, Ty_Int());
					//如果是array或者record需要检查每个成员的类型
					if (left.ty->kind == Ty_array&&right.ty->kind == Ty_array) {
						if(actualTy(left.ty->u.array)== actualTy(right.ty->u.array))
							return expTy(NULL, Ty_Int());
					}
					if(left.ty->kind == Ty_record&&right.ty->kind == Ty_record) {
						//注意这里
						//type a={x:int,y:int};
						//type b={x:int,y:int};
						//var x:a:=...
						//var y:b:=...
						//x=y是非法的
						if (actualTy(left.ty->u.record) == actualTy(right.ty->u.record))
							return expTy(NULL, Ty_Int());
					}
					EM_error(a->u.op.right->pos, "left and right must belong to the same type\n");
					assert(0);
				}
			}			
			
		}
		case A_varExp:{
			return transVar(venv,tenv, v->u.var);
		} 
		case A_nilExp:{
			return expTy(NULL,Ty_Nil());
		} 
		case A_intExp:{
			return expTy(NULL, Ty_Int());
		} 
		case A_stringExp:{
			return expTy(NULL, Ty_String());
		} 
		case A_callExp:{
			//检查function是否已经声明
			E_enventry x = (E_enventry)S_table(tenv, v->u.call.func);
			if (!x || x->kind != E_funEntry)
			{
				EM_error(v->pos, "undefined function %s",
					S_name(v->u.call.func));
				assert(0);
			}
			//检查参数是否符合规范
			while (x->u.fun.formals->head!=NULL&& v->u.call.args->head!=NULL) {
				tmp_expty=transExp(venv, tenv, v->u.call.args->head);
				if (actualTy((x->u.fun.formals->head) != actualTy(tmp_expty.ty))) {
					EM_error(v->pos, "invalid args %s",
						S_name(v->u.call.func));
					assert(0);
				}
				v->u.call.args = v->u.call.args->tail;
				x->u.fun.formals = x->u.fun.formals->tail;
			}
			//如果任意一个指针不为空说明参数与函数要求参数不等长
			if (x->u.fun.formals->head != NULL|| v->u.call.args->head != NULL) {
				EM_error(v->pos, "invalid args%s",
					S_name(v->u.call.func));
				assert(0);
			}
			return expTy(NULL,actualTy(x->u.fun.result));
		}
	    case A_recordExp:{
			//type{id1=exp1,id2=exp2...}
			//检查type, id与exp是否匹配，以及是否将type中成员遍历完
			Ty_ty type = (Ty_ty)S_look(tenv, v->u.record.typ);
			if (type == NULL||actualTy(type)!=Ty_record) {
				EM_error(v->pos, "type doesn't exist\n");
				assert(0);
			}
			while (type->u.record->head&&v->u.record.fields->head) {
				//检查id是否存在
				if (type->u.record->head->name != v->u.record.fields->head->name) {
					EM_error(v->pos, "member doesn't exist\n");
					assert(0);
				}
				//检查exp与id类型是否相同
				tmp_expty = transExp(venv, tenv, v->u.record.fields->head->exp);
				if (actualTy(type->u.record->head->ty) != actualTy(tmp_expty->ty)) {
					EM_error(v->pos, "type doesn't match\n");
					assert(0);
				}
				type->u.record = type->u.record->tail;
				v->u.record.fields = v->u.record.fields->tail;
			}
			//record必须全部赋值完
			if (type->u.record->head || v->u.record.fields->head) {
				EM_error(v->pos, "record error\n");
				assert(0);
			}
			return expTy(NULL, actualTy(ty));
		}
		case A_seqExp:{
			//{exp1;exp2;exp3...}
			//检查每一个子exp的正确性
			A_expList explist = v->u.seq;
			while (explist->head != NULL) {
				tmp_expty = transExp(venv, tenv, explist->head);
				//{exp1;exp2;exp3...}应该不返回任何值,但let语句要求返回最后一个语句的值，因此exps要返回最后一个语句的类型
				if (explist->tail==NULL)
					return tmp_expty;
				explist = explist->tail;
			}
			
			return expTy(NULL,Ty_Void());
		} 
		case A_assignExp:{
			//先检查左边var的正确性，再检查右边exp的正确性			
			tmp_expty = transVar(venv, tenv, v->u.assign.var);
			struct expty tmp_expty2 = transExp(venv, tenv, v->u.assign.exp);
			//最后左右两端的类型应该一样
			if (tmp_expty.ty->kind != tmp_expty2.ty->kind) {
				if (actualTy(tmp_expty.ty) != actualTy(tmp_expty2.ty)) {
					EM_error(v->pos, "invalid assign");
					assert(0);
				}
			}
			//应该不返回任何值
			return expTy(NULL, Ty_Void());
		} 
		case A_ifExp:{
			//if exp1 then exp2 else exp3
			//if exp1 then exp2
			//检查if exp else exp then exp 中3个exp
			//此外第一个exp返回值必须是Int型
			tmp_expty= transExp(venv, tenv, v->u.iff.test);
			if (!tmp_expty || tmp_expty.ty != Ty_int) {
				EM_error(v->pos, "wrong value of if()");
				assert(0);
			}
			struct expty tmp_expty2 = transExp(venv, tenv, v->u.iff.then);
			if (v->u.iff.elsee != NULL) {
				//if then else类型，根据定义，exp2与exp3必须类型相同
				struct expty tmp_expty3 = transExp(venv, tenv, v->u.iff.elsee);
				if (actualTy(tmp_expty2.ty) != actualTy(tmp_expty3.ty)) {
					EM_error(v->pos, "then and else must return the same type\n");
					assert(0);
				}
				return expTy(NULL, actualTy(tmp_expty2.ty));
			}
			//if then类型，根据定义应该不返回任何值
			if (actualTy(tmp_expty2.ty) != Ty_Void()) {
				EM_error(v->pos, "then must return void type\n");
				assert(0);
			}
			return expTy(NULL, Ty_Void());
		}
	    case A_whileExp:{
			//while exp1 do exp2
			//先检查exp1是否返回整数值，再检查exp2是否无值
			tmp_expty = transExp(venv, tenv, v->u.whilee.test);
			if (!tmp_expty || tmp_expty.ty != Ty_int) {
				EM_error(v->pos, "wrong value of while()");
				assert(0);
			}
			struct expty tmp_expty2 = transExp(venv, tenv, v->u.whilee.body);
			if (actualTy(tmp_expty2) != Ty_void) {
				EM_error(v->pos, "while body must be void type");
				assert(0);
			}
			return expTy(NULL, Ty_Void());
		} 
		case A_forExp:{
			// for id:exp1 to exp2 do exp3
			//检查id/exp1与exp2是否返回整数值，exp3是否无值
			//id 在exp3内不能被修改
			//绑定变量id的作用域
			tmp_expty = transExp(venv,tenv,v->u.forr.lo);
			struct expty tmp_expty = transExp(venv, tenv, v->u.forr.hi);
			if (actualTy(tmp_expty.ty) != Ty_int || actualTy(tmp_expty2.ty) != Ty_int) {
				EM_error(v->pos, "error in loop index\n");
				assert(0);
			}
			//该部分需要将id的值压入值环境中,因为id是Int型，不需要绑定类型环境
			S_beginScope(venv);
			S_enter(venv, v->u.forr.var, E_VarEntry(tmp_expty.ty));
			//body是否返回void
			tmp_expty = transExp(venv, tenv, v->u.forr.body);
			if (actualTy(tmp_expty.ty) != Ty_Void) {
				EM_error(v->pos, "body must return void\n");
				assert(0);
			}
			S_endScope(venv);
			return expTy(NULL,Ty_Void());

		}
		case A_breakExp:{
			return expTy(NULL, Ty_Void());
		}
		case A_letExp:{
			//let decs in exps end
			//绑定类型，变量的作用域
			//let 最后一个表达式结果将作为整个结果
			S_beginScope(venv);
			S_beginScope(tenv);
			while (v->u.let.decs->head != NULL) {
				transDec(venv,tenv,v->u.let.decs->head);
				v->u.let.decs = v->u.let.decs->tail;
			}
			tmp_expty = transExp(venv, tenv, v->u.let.body);
			S_endScope(tenv);
			S_endScope(venv);
			if (tmp_expty == NULL)
				return expTy(NULL, Ty_Void());
			return tmp_expty;
		}
		case A_arrayExp:{
			//type[exp1] of exp2
			//检查type是否存在，exp1返回是否是整数，exp2返回是否是type类型
			Ty_ty type = (Ty_ty)S_look(tenv, v->u.array.typ);
			if (type == NULL) {
				EM_error(v->pos, "type doesn't exist\n");
				assert(0);
			}
			tmp_expty = transExp(venv, tenv, v->u.array.size);
			if (tmp_expty == NULL || tmp_expty.ty != Ty_int) {
				EM_error(v->pos, "size must be int\n");
				assert(0);
			}
			struct expty tmp_expty2 = transExp(venv, tenv, v->u.array.init);
			if (tmp_expty == NULL || actualTy(tmp_expty.ty) != actualTy(tmp_expty2.ty)) {
				EM_error(v->pos, "initialize error\n");
				assert(0);
			}
			return expTy(NULL, actualTy(type));
		}
	}
	assert(0);
}
//***********************************
//TODO: the declaration is unfinished
//***********************************
void transDec(S_table venv,S_table tenv,A_dec d)
{
	switch(d->kind){
		case A_varDec:{
			//var id:=exp
			//var id:type-id:=exp
			//首先exp必须合法且返回类型与type-id相同
			//特殊的，id不能等于"int"或者"string"
			if (d->u.var.init == NULL) {
				EM_error(v->pos, "var declaration must be initialize\n");
				assert(0);
			}
			struct expty e= transExp(venv,tenv,d->u.var.init);
			if (e.ty!=NULL&&actualTy(e.ty) != d->u.var.typ) {
				EM_error(v->pos, "var declaration type error\n");
				assert(0);
			}
			if (S_name(d->u.var.var) == "int" || S_name(d->u.var.var) == "string") {
				EM_error(v->pos, "var declaration use invalid id\n");
				assert(0);
			}
			S_enter(venv,d->u.var.var,E_VarEntry(e.ty));
			
		}
		case A_typeDec:{
			//type type-id=ty
			//ty=type-id
			//ty={type-fields}
			//ty=array of type-id
			//首先避免定义int 或string等基本类型
			//查找每个type是否存在,如果存在则要将其扔进venv中
			A_nametyList namelist = d->u.type;
			while (namelist->head!=NULL) {
				if (S_name(namelist->head->name) == "int" || S_name(namelist->head->name) == "string") {
					EM_error(v->pos, "type declaration use invalid id\n");
					assert(0);
				}
				Ty_ty ty = transTy(tenv,d->u.type->head->ty);
				//防止递归
				if(actualTy(ty)==NULL)
				{
					EM_error(v->pos, "type declaration use unknown type-id\n");
					assert(0);
				}
				S_enter(tenv, namelist->head->name,
					transTy(namelist->head->ty));
				namelist = namelist->tail;
			}
			
		}
		case A_functionDec:{
			//function id(type-fields):=exp
			//function id(type-fields):type-id=exp
			//将type-fields进行检查，并且将函数压入venv中
			//将type-fields声明的变量压入venv中
			//注意函数名不能为"int"或者"string"
			while (d->u.function->head) {
				A_fundec f = d->u.function->head;
				Ty_ty resultTy = S_look(tenv, f->result);
				//Ty_tyList formalTys = makeFormalTyList(tenv, f->params);
				Ty_tyList formalTys;
				A_fieldList tmpfieldList =f->params;
				if (S_name(f->name) == "int" || S_name(f->name) == "string") {
					EM_error(v->pos, "function declaration use invalid id\n");
					assert(0);
				}
				while (tmpfieldList->head) {
					Ty_ty ty = (Ty_ty)S_look(tenv, tmpfieldList->head->typ);
					if (ty == NULL) {
						EM_error(v->pos, "function declaration has unknown type\n");
						assert(0);
					}
					tylist = Ty_TyList(ty, tylist);
					tmpfieldList = tmpfieldList->tail;
				}
				S_enter(venv, f->name, E_FunEntry(formalTys, resultTy));
				S_beginScope(venv);
				{
					A_fieldList l; Ty_tyList t;
					for (l = f->params, t = formalTys; l; l = l->tail, t = t->tail)
						S_enter(venv, l->head->name, E_VarEntry(t->head));
				}
				transExp(venv, tenv, d->u.function->body);
				S_beginScope(venv);
				d->u.function = d->u.function->tail;
			}
		}
	}
}
Ty_ty transTy(S_table tenv,A_ty v)
{
	switch(v->kind){
		case A_nameTy:{
			if (v->u.name == "int")
				return Ty_Int();
			if (v->u.name == "string")
				return Ty_String();
			//如果不是基础类型需要进入类型环境搜索相应类型
			Ty_ty type = (Ty_ty )S_look(tenv, v->u.name);
			if (type == NULL) {
				EM_error(a->u.op.left->pos, "The type doesn't exist\n");
				asser(0);
			}
			return Ty_Name(v->u.name,type);
		} 
		case A_recordTy:{
			//需要检查每一个子类型
			Ty_fieldList fieldlist = NULL;
			while (v->u.record->head != NULL) {
				Ty_ty type = (Ty_ty )S_look(tenv,v->u.record->head->typ);
				if (type == NULL) {
					EM_error(a->u.op.left->pos, "The type doesn't exist\n");
					asser(0);
				}
				v->u.record = v->u.record->tail;
				fieldlist = Ty_FieldList(Ty_Field(v->u.record->head->name,type),fieldlist);
			}
			return Ty_Record(fieldlist);
		} 
		case A_arrayTy:{
			//需要检查子类型是否满足要求
			Ty_ty type = (Ty_ty )S_look(tenv, v->u.array);
			if (type == NULL) {
				EM_error(a->u.op.left->pos, "The type of array doesn't exist\n");
				asser(0);
			}
			return Ty_Array(type);
		}
	}
}