#include <stdio.h>
#include <string.h>
#include <io.h>
#include "semant.h"
#include "parse.h"
#include "prabsyn.h"
#include "translate.h"
//IR,we temporarily define it as void*

struct expty expTy(Tr_exp exp, Ty_ty ty){
	struct expty e;
	e.exp=exp;
	e.ty=ty;
	return e;
}
U_boolList makeFormalEscapeList(A_fieldList params) {
	A_fieldList fieldList = params;
	U_boolList boolList = NULL;
	if (fieldList && fieldList->head) {
		boolList = U_BoolList(fieldList->head->escape, NULL);
		fieldList = fieldList->tail;
	}
	U_boolList temp = boolList;
	while (fieldList && fieldList->head) {
		temp->tail = U_BoolList(fieldList->head->escape, NULL);
		temp = temp->tail;
		fieldList = fieldList->tail;
	}
	return boolList;
}
struct expty transVar(S_table venv,S_table tenv,A_var v,Tr_level level, Temp_label done)
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
			return expTy(Tr_simpleVar(x->u.var.access,level), actualTy(x->u.var.ty));
		}
		case A_fieldVar:{
			//遇到A.b这种形式我们需要先检查A是否合乎语义
			tmp_expty = transVar(venv, tenv, v->u.field.var,level,done);
			//A的类型必定是ty_record，即必须是fieldlist,否则报错
			if (!tmp_expty.ty||tmp_expty.ty->kind != Ty_record) {
				EM_error(v->pos, "undefined FieldList");
				assert(0);
			}
			//如果是field list需要遍历fieldlist寻找到该值
			Ty_fieldList record = tmp_expty.ty->u.record;
			if (record == NULL) {
				EM_error(v->pos, "undefined FieldList");
				assert(0);
			}
			//记录record第n个成员
			int count = 0;
			while (record->head->name != v->u.field.sym) {
				record = record->tail;
				if (record == NULL) {
					EM_error(v->pos, "undefined member in FieldList");
					assert(0);
				}
				count++;
			}
			return expTy(Tr_fieldVar(tmp_expty.ty, count), 
				actualTy(record->head->ty));
		}
		case A_subscriptVar:{
			
			tmp_expty =transVar(venv,tenv,v->u.subscript.var,level,done);
			//如果不是数组，则报错
			if(tmp_expty.ty->kind!=Ty_array){
				EM_error(v->pos,"undefined array");
				assert(0);
			}
			struct expty tmp_expty_exp =transExp(venv,tenv,v->u.subscript.exp,level,done);
			//如果数组内返回值不是int
			if(tmp_expty_exp.ty->kind!=Ty_int){
				EM_error(v->pos,"The index of array is not int");
				assert(0);
			}
			return  expTy(Tr_subscriptVar(tmp_expty.exp, tmp_expty_exp.exp),
				actualTy(tmp_expty.ty->u.array));
		}
	}
}
struct expty transExp(S_table venv,S_table tenv,A_exp v,Tr_level level, Temp_label done)
{
	struct expty tmp_expty;
	switch(v->kind){
		case A_opExp:{
			A_oper oper=v->u.op.oper;
			struct expty left=transExp(venv,tenv,v->u.op.left, level, done);
			struct expty right=transExp(venv,tenv,v->u.op.right, level, done);
			switch (oper) {
				case A_plusOp:
				case A_minusOp:
				case A_timesOp:
				case A_divideOp:
				case A_ltOp:
				case A_leOp:
				case A_gtOp:
				case A_geOp: {
					//上述操作符都需要整数运算
					if (left.ty->kind != Ty_int) {
						EM_error(v->u.op.left->pos, "integer required");
						assert(0);
					}
					if (right.ty->kind != Ty_int) {
						EM_error(v->u.op.right->pos, "integer required");
						assert(0);
					}
					return expTy(Tr_opExp(oper,left.exp,right.exp), Ty_Int());
				}
				case A_eqOp:
				case A_neqOp: {					
					if(left.ty->kind==Ty_int&&right.ty->kind == Ty_int)
						return expTy(Tr_opExp(oper, left.exp, right.exp), Ty_Int());
					if (left.ty->kind == Ty_string&&right.ty->kind == Ty_string)
						return expTy(Tr_opExp(oper, left.exp, right.exp), Ty_Int());
					//如果是array或者record需要检查每个成员的类型
					if (left.ty->kind == Ty_array&&right.ty->kind == Ty_array) {
						//if(actualTy(left.ty->u.array)== actualTy(right.ty->u.array))
						if (TyEqual(left.ty->u.array,right.ty->u.array))
							return expTy(Tr_opExp(oper, left.exp, right.exp), Ty_Int());
					}
					if(left.ty->kind == Ty_record) {
						//注意这里
						//type a={x:int,y:int};
						//type b={x:int,y:int};
						//var x:a:=...
						//var y:b:=...
						//x=y是非法的
						if (right.ty->kind == Ty_record) {
							if (TyEqual(left.ty->u.record, right.ty->u.record))
								return expTy(Tr_opExp(oper, left.exp, right.exp), Ty_Int());
						}
						//如果右边是nil，也合法
						if (right.ty->kind==Ty_nil)
							return expTy(Tr_opExp(oper, left.exp, right.exp), Ty_Int());
					}
					EM_error(v->u.op.right->pos, "left and right must belong to the same type\n");
					assert(0);
				}
			}			
			
		}
		case A_varExp:{
			return transVar(venv,tenv, v->u.var,level,done);
		} 
		case A_nilExp:{
			return expTy(Tr_nilExp(),Ty_Nil());
		} 
		case A_intExp:{
			return expTy(Tr_intExp(v->u.intt), Ty_Int());
		} 
		case A_stringExp:{
			return expTy(Tr_stringExp(v->u.stringg), Ty_String());
		} 
		case A_callExp:{
			//检查function是否已经声明
			E_enventry x = (E_enventry)S_look(venv, v->u.call.func);
			if (!x || x->kind != E_funEntry)
			{
				EM_error(v->pos, "undefined function %s",
					S_name(v->u.call.func));
				assert(0);
			}
			//检查参数是否符合规范
			Ty_tyList formals= x->u.fun.formals;
			A_expList args = v->u.call.args;
			Tr_expList tr_explist = NULL;
			while (formals!=NULL&& args!=NULL) {
				tmp_expty=transExp(venv, tenv, args->head, level, done);
				if (actualTy(formals->head)->kind != actualTy(tmp_expty.ty)->kind) {
				EM_error(v->pos, "invalid args to call function %s",
						S_name(v->u.call.func));
					assert(0);
				}
				tr_explist = Tr_ExpList(tmp_expty.exp,tr_explist);
				args = args->tail;
				formals = formals->tail;
			}
			//如果任意一个指针不为空说明参数与函数要求参数不等长
			if (formals!= NULL|| args!= NULL) {
				EM_error(v->pos, "args and formals must be the same length to call %s",
					S_name(v->u.call.func));
				assert(0);
			}
			return expTy(Tr_callExp(x->u.fun.label,tr_explist,x->u.fun.level,level),
				actualTy(x->u.fun.result));
		}
	    case A_recordExp:{
			//type{id1=exp1,id2=exp2...}
			//检查type, id与exp是否匹配，以及是否将type中成员遍历完
			Ty_ty type = (Ty_ty)S_look(tenv, v->u.record.typ);
			type = actualTy(type);
			if (type == NULL||actualTy(type)==NULL|| actualTy(type)->kind!=Ty_record) {
				EM_error(v->pos, "type doesn't exist\n");
				assert(0);
			}
			A_efieldList tmp_fieldlist=v->u.record.fields;
			Ty_fieldList tmp_record = type->u.record;
			Tr_expList tr_explist = NULL;
			while (tmp_record&&tmp_fieldlist) {
				//检查id是否存在,这里只能是顺序检查
				if (tmp_record->head->name != tmp_fieldlist->head->name) {
					EM_error(v->pos, "member doesn't exist\n");
					assert(0);
				}
				//检查exp与id类型是否相同
				tmp_expty = transExp(venv, tenv, tmp_fieldlist->head->exp, level, done);
				if (actualTy(tmp_record->head->ty)->kind != actualTy(tmp_expty.ty)->kind) {
					if (actualTy(tmp_record->head->ty)->kind != Ty_record || actualTy(tmp_expty.ty)->kind != Ty_nil) {
						EM_error(v->pos, "type doesn't match\n");
						assert(0);
					}
				}
				tr_explist = Tr_ExpList(tmp_expty.exp, tr_explist);
				tmp_record = tmp_record->tail;
				tmp_fieldlist = tmp_fieldlist->tail;
			}
			//record必须全部赋值完
			if (tmp_record!=NULL || tmp_fieldlist!=NULL) {
				EM_error(v->pos, "record error\n");
				assert(0);
			}
			return expTy(Tr_recordExp(tr_explist), actualTy(type));
		}
		case A_seqExp:{
			//{exp1;exp2;exp3...}
			//检查每一个子exp的正确性
			A_expList explist = v->u.seq;
			Tr_expList tr_explist = NULL;
			if (explist) {
				while (explist->tail) {
					tmp_expty = transExp(venv, tenv, explist->head, level, done);
					tr_explist = Tr_ExpList(tmp_expty.exp, tr_explist);
					explist = explist->tail;
				}
			}
			else {
				return expTy(Tr_seqExp(NULL,false), Ty_Void());
			}
			tmp_expty = transExp(venv, tenv, explist->head, level, done);
			tr_explist = Tr_ExpList(tmp_expty.exp, tr_explist);
			return expTy(Tr_seqExp(tr_explist,true),tmp_expty.ty);
		} 
		case A_assignExp:{
			//先检查左边var的正确性，再检查右边exp的正确性			
			tmp_expty = transVar(venv, tenv, v->u.assign.var,level,done);
			struct expty tmp_expty2 = transExp(venv, tenv, v->u.assign.exp,level,done);
			//最后左右两端的类型应该一样
			if (tmp_expty.ty->kind != tmp_expty2.ty->kind) {
				if (TyEqual(tmp_expty.ty,tmp_expty2.ty)==false|| tmp_expty.ty->kind==Ty_nil) {
					EM_error(v->pos, "invalid assign");
					assert(0);
				}
			}
			//应该不返回任何值
			return expTy(Tr_assignExp(tmp_expty.exp,tmp_expty2.exp),
				Ty_Void());
		} 
		case A_ifExp:{
			//if exp1 then exp2 else exp3
			//if exp1 then exp2
			//检查if exp else exp then exp 中3个exp
			//此外第一个exp返回值必须是Int型
			tmp_expty= transExp(venv, tenv, v->u.iff.test, level, done);
			if (!tmp_expty.ty || tmp_expty.ty->kind != Ty_int) {
				EM_error(v->pos, "wrong value of if()");
				assert(0);
			}
			struct expty tmp_expty2 = transExp(venv, tenv, v->u.iff.then,level,done);
			if (v->u.iff.elsee != NULL) {
				//if then else类型，根据定义，exp2与exp3必须类型相同
				struct expty tmp_expty3 = transExp(venv, tenv, v->u.iff.elsee, level, done);
				if (actualTy(tmp_expty2.ty) != actualTy(tmp_expty3.ty)) {
					EM_error(v->pos, "then and else must return the same type\n");
					assert(0);
				}
				return expTy(Tr_ifExp(tmp_expty.exp,tmp_expty2.exp,tmp_expty3.exp), 
					actualTy(tmp_expty2.ty));
			}
			//if then类型，根据定义应该不返回任何值
			if (actualTy(tmp_expty2.ty) != Ty_Void()) {
				EM_error(v->pos, "then must return void type\n");
				assert(0);
			}
			return expTy(Tr_ifExp(tmp_expty.exp, tmp_expty2.exp, NULL),
				Ty_Void());
		}
	    case A_whileExp:{
			//while exp1 do exp2
			//先检查exp1是否返回整数值，再检查exp2是否无值
			Temp_label done = Temp_newlabel();
			tmp_expty = transExp(venv, tenv, v->u.whilee.test,level,done);
			if (!tmp_expty.ty || tmp_expty.ty->kind != Ty_int) {
				EM_error(v->pos, "wrong value of while()");
				assert(0);
			}
			struct expty tmp_expty2 = transExp(venv, tenv, v->u.whilee.body, level, done);
			if (actualTy(tmp_expty2.ty)->kind != Ty_void) {
				EM_error(v->pos, "while body must be void type");
				assert(0);
			}
			return expTy(Tr_whileExp(tmp_expty.exp,tmp_expty2.exp,done), 
				Ty_Void());
		} 
		case A_forExp:{
			// for id:exp1 to exp2 do exp3
			//检查id/exp1与exp2是否返回整数值，exp3是否无值
			//id 在exp3内不能被修改
			//绑定变量id的作用域
			Temp_label done = Temp_newlabel();
			tmp_expty = transExp(venv,tenv,v->u.forr.lo, level, done);
			Tr_access acc = Tr_allocLocal(level,v->u.forr.escape);
			struct expty tmp_expty2 = transExp(venv, tenv, v->u.forr.hi, level, done);
			if (actualTy(tmp_expty.ty)->kind != Ty_int || actualTy(tmp_expty2.ty)->kind != Ty_int) {
				EM_error(v->pos, "error in loop index\n");
				assert(0);
			}
			//该部分需要将id的值压入值环境中,因为id是Int型，不需要绑定类型环境
			S_beginScope(venv);
			S_enter(venv, v->u.forr.var, E_VarEntry(acc,tmp_expty.ty));
			//body是否返回void
			struct expty tmp_expty3 = transExp(venv, tenv, v->u.forr.body, level, done);
			if (actualTy(tmp_expty3.ty)->kind != Ty_void) {
				EM_error(v->pos, "body must return void\n");
				assert(0);
			}
			S_endScope(venv);
			return expTy(Tr_forExp(Tr_simpleVar(acc,level),tmp_expty.exp,
				tmp_expty2.exp,tmp_expty3.exp,done),
				Ty_Void());

		}
		case A_breakExp:{
			if (!done) {
				EM_error(v->pos, "Break is not in a loop");
				return expTy(Tr_Nop(), Ty_Void());
			}
			return expTy(Tr_breakExp(done), Ty_Void());
		}
		case A_letExp:{
			//let decs in exps end
			//绑定类型，变量的作用域
			//let 最后一个表达式结果将作为整个结果
			S_beginScope(venv);
			S_beginScope(tenv);
			A_decList tmp_declist = v->u.let.decs;
			Tr_expList tr_explist = NULL;
			while (tmp_declist != NULL) {
				tr_explist=Tr_ExpList(
					transDec(venv,tenv, tmp_declist->head, level, done),
					tr_explist);
				tmp_declist = tmp_declist->tail;
			}
			if (v->u.let.body) {
				tmp_expty = transExp(venv, tenv, v->u.let.body, level, done);
				tmp_expty = expTy(Tr_letExp(tr_explist, tmp_expty.exp), actualTy(tmp_expty.ty));
			}
			else {
				tmp_expty = expTy(Tr_letExp(tr_explist, NULL), Ty_Void());
			}
			S_endScope(tenv);
			S_endScope(venv);
			return tmp_expty;
		}
		case A_arrayExp:{
			//type[exp1] of exp2
			//检查type是否存在，exp1返回是否是整数，exp2返回是否是type类型
			Ty_ty type = (Ty_ty)S_look(tenv, v->u.array.typ);
			if (type == NULL||actualTy(type)->kind!= Ty_array) {
				EM_error(v->pos, "array type doesn't exist\n");
				assert(0);
			}
			tmp_expty = transExp(venv, tenv, v->u.array.size, level, done);
			if (tmp_expty.ty == NULL || tmp_expty.ty != Ty_Int()) {
				EM_error(v->pos, "array size must be int\n");
				assert(0);
			}
			struct expty tmp_expty2 = transExp(venv, tenv, v->u.array.init, level, done);
			if (tmp_expty.ty == NULL || actualTy(actualTy(type)->u.array)->kind != actualTy(tmp_expty2.ty)->kind) {
				EM_error(v->pos, "array initialize error\n");
				assert(0);
			}
			return expTy(Tr_arrayExp(tmp_expty.exp,tmp_expty2.exp), 
				actualTy(type));
		}
	}
	assert(0);
}
//***********************************
//TODO: the declaration is unfinished
//***********************************
Tr_exp transDec(S_table venv,S_table tenv,A_dec d,Tr_level level, Temp_label done)
{
	switch(d->kind){
		case A_varDec:{
			//var id:=exp
			//var id:type-id:=exp
			//首先exp必须合法且返回类型与type-id相同
			//特殊的，id不能等于"int"或者"string"
			//在没有type-id情况下不能初始化为nil
			if (d->u.var.init == NULL) {
				EM_error(d->pos, "var declaration must be initialize\n");
				assert(0);
			}
			struct expty e= transExp(venv,tenv,d->u.var.init, level, done);
			if (d->u.var.typ) {
				if (e.ty == NULL||!TyEqual(e.ty, (Ty_ty)S_look(tenv, d->u.var.typ))) {
					EM_error(d->pos, "var declaration type error\n");
					assert(0);
				}
			}
			else {
				if (e.ty->kind == Ty_nil) {
					EM_error(d->pos, "var can't be nil without record type\n");
					assert(0);
				}
			}
			if (S_name(d->u.var.var) == "int" || S_name(d->u.var.var) == "string") {
				EM_error(d->pos, "var declaration use invalid id\n");
				assert(0);
			}
			//如果有var a:type-id:=nil的情况，记录原始类型
			Tr_access acc = Tr_allocLocal(level, d->u.var.escape);
			if(e.ty->kind!=Ty_nil)
				S_enter(venv,d->u.var.var,E_VarEntry(acc,e.ty));
			else
				S_enter(venv, d->u.var.var, E_VarEntry(acc,S_look(tenv, d->u.var.typ)));
			return Tr_assignExp(Tr_simpleVar(acc, level), e.exp);
			break;
		}
		case A_typeDec:{
			//type type-id=ty
			//ty=type-id
			//ty={type-fields}
			//ty=array of type-id
			//首先避免定义int 或string等基本类型
			//查找每个type是否存在,如果存在则要将其扔进venv中

			//为了支持递归定义，要先扫一次decs再检查子类型
			A_nametyList namelist = d->u.type;
			//先给类型头赋值为Ty_Name压入环境
			int count = 0;
			char* tmpname[50];
			while (namelist) {
				if (S_name(namelist->head->name) == "int" || S_name(namelist->head->name) == "string") {
					EM_error(d->pos, "type declaration use invalid id\n");
					assert(0);
				}
				//去重
				for (int i = 0; i < count; i++) {
					if (strcmp(S_name(namelist->head->name), tmpname[i])== 0) {
						EM_error(d->pos, "type %s redefine\n", S_name(namelist->head->name));
						assert(0);
					}
				}
				tmpname[count] = S_name(namelist->head->name);
				count++;
				S_enter(tenv, namelist->head->name,
					Ty_Name(namelist->head->name,NULL));
				namelist = namelist->tail;
			}
			namelist = d->u.type;
			bool isCircle = TRUE;
			while (namelist) {
				Ty_ty ty = transTy(tenv, namelist->head->ty);
				Ty_ty ty_in_table = (Ty_ty)S_look(tenv, namelist->head->name);
				//修改子成员类型		
				if (ty->kind != Ty_name)
					isCircle = FALSE;
				ty_in_table->u.name.ty = ty;	
				namelist = namelist->tail;
			}
			if (isCircle) {
				EM_error(d->pos, "recursive type define \n");
				assert(0);
			}
			return Tr_Nop();
			break;
		}
		case A_functionDec:{
			//function id(type-fields):=exp
			//function id(type-fields):type-id=exp
			//将type-fields进行检查，并且将函数压入venv中
			//将type-fields声明的变量压入venv中
			//检查输出类型
			//注意函数名不能为"int"或者"string"
			A_fundecList fundecList = d->u.function;
			char*tmp_function_name[50];
			int count = 0;
			while (fundecList) {
				A_fundec f = fundecList->head;
				//去重
				for (int i = 0; i < count; i++) {
					if (strcmp(S_name(f->name), tmp_function_name[i]) == 0) {
						EM_error(d->pos, "function %s redefine \n", S_name(f->name));
						assert(0);
					}
				}
				
				Ty_ty resultTy = NULL;
				A_fieldList tmpfieldList = f->params;
				Ty_tyList formalTys = NULL;
				U_boolList l = makeFormalEscapeList(f->params);
				Temp_label label = Temp_newlabel();
				Tr_level cur = Tr_newLevel(level, label, l);
				//函数名字不能为下列名字
				if (S_name(f->name) == "int" || S_name(f->name) == "string") {
					EM_error(d->pos, "function declaration use invalid id\n");
					assert(0);
				}
				//如果没有result，默认为void
				if (!f->result)
					resultTy = Ty_Void();
				else {
					resultTy = (Ty_ty)S_look(tenv, f->result);
					if (!resultTy) {
						EM_error(d->pos, "function return value undefined\n");
						assert(0);
					}
				}
				while (tmpfieldList) {
					//查看每个参数，构造formal与result
					//检查该类型是否存在
					Ty_ty ty = (Ty_ty)S_look(tenv, tmpfieldList->head->typ);
					if (ty == NULL) {
						EM_error(d->pos, "function declaration has unknown type\n");
						assert(0);
					}
					formalTys = Ty_TyList(ty, formalTys);
					tmpfieldList = tmpfieldList->tail;
				}
				formalTys = Ty_TyList_reverse(formalTys);
				S_enter(venv, f->name, E_FunEntry(cur, label,formalTys, resultTy));
				tmp_function_name[count] = S_name(f->name);
				fundecList = fundecList->tail;
				count++;
			}
			fundecList = d->u.function;
			Tr_expList tr_explist = NULL;
			while (fundecList)
			{
				A_fundec f = fundecList->head;
				E_enventry enventry = (E_enventry)S_look(venv, f->name);
				Ty_tyList formalTys = enventry->u.fun.formals;
				Ty_ty resultTy = enventry->u.fun.result;
				Tr_level curLevel =enventry->u.fun.level;
				Tr_accessList paramsAcc = Tr_formals(curLevel);
				S_beginScope(venv);
				{
					A_fieldList l; Ty_tyList t;
					Tr_accessList accessList;
					for (l = fundecList->head->params, t = formalTys,accessList=paramsAcc; l;
						l = l->tail, t = t->tail,accessList=accessList->tail)
						S_enter(venv, l->head->name, E_VarEntry(accessList->head,t->head));
				}
				struct expty tmp_expty=transExp(venv, tenv, fundecList->head->body, level, done);
				S_endScope(venv);
				fundecList = fundecList->tail;
				if (!tmp_expty.ty||actualTy(tmp_expty.ty)->kind != resultTy->kind) {
					EM_error(d->pos, "function declaration type error\n");
					assert(0);
				}
				//检查输出结果类型
				tr_explist = Tr_ExpList(tmp_expty.exp, tr_explist);
				Tr_procEntryExit(curLevel,tmp_expty.exp,paramsAcc);
			}
			Tr_exp result= Tr_funDec(tr_explist);
			return result;
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
			if (v->u.name == "void")
				return Ty_Void();
			if (v->u.name == "nil")
				return Ty_Nil();
			//如果不是基础类型需要进入类型环境搜索相应类型
			Ty_ty type = (Ty_ty )S_look(tenv, v->u.name);
			if (type == NULL) {
				EM_error(v->pos, "The type doesn't exist\n");
				assert(0);
			}
			return type;
		} 
		case A_recordTy:{
			//需要检查每一个子类型
			Ty_fieldList fieldlist = NULL;
			A_fieldList tmplist = v->u.record;
			while (tmplist != NULL) {
				Ty_ty type = (Ty_ty )S_look(tenv, tmplist->head->typ);
				if (type == NULL) {
					EM_error(v->pos, "The type doesn't exist\n");
					assert(0);
				}
				fieldlist = Ty_FieldList(Ty_Field(tmplist->head->name, type),fieldlist);
				tmplist = tmplist->tail;
			}
			return Ty_Record(Ty_FieldList_reverse(fieldlist));
		} 
		case A_arrayTy:{
			//需要检查子类型是否满足要求
			Ty_ty type = (Ty_ty )S_look(tenv, v->u.array);
			if (type == NULL) {
				EM_error(v->pos, "The type of array doesn't exist\n");
				assert(0);
			}
			return Ty_Array(type);
		}
	}
}
