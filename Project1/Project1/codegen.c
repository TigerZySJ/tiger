#include "codegen.h"
static void munchStm(T_stm s);
static Temp_temp munchExp(T_exp s);
static AS_instrList assem_instr = NULL;
//利用Maxinum Munch算法产生assem算法
AS_instrList F_codegen(F_frame frame, T_stmList stmList) {
	T_stmList tmp = NULL;
	for (tmp = stmList; tmp; tmp = tmp->tail) {
		munchStm(tmp->head);
	}
	return assem_instr;
}
//添加assem指令
static void emit(AS_instr instr) {
	static AS_instrList cur = NULL;
	if (!assem_instr) {
		cur=assem_instr = AS_InstrList(instr,NULL);
	}
	else {
		cur=cur->tail = AS_InstrList(instr, NULL);
	}
}
//Maxinum Munch分析stm
static void munchStm(T_stm s) {
	char* assem=(char*)checked_malloc(sizeof(char)*100);
	Temp_temp r = Temp_newtemp();
	switch (s->kind) {
		case T_SEQ: {
			T_stm e1 = s->u.SEQ.left;
			T_stm e2 = s->u.SEQ.right;
			munchExp(e1); munchExp(e2);
			break;
		} 
		case T_LABEL: {
			sprintf(assem,"%s\n",Temp_labelstring(s->u.LABEL));
			emit(AS_Label(assem, s->u.LABEL));
			break;
		}
		case T_JUMP: {
			Temp_temp r = munchExp(s->u.JUMP.exp);
			sprintf(assem, "jmp %s\n", Temp_labelstring(s->u.JUMP.jumps->head));
			emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, AS_Targets(s->u.JUMP.jumps)));
			//emit("jump");
			break;
		}
		case T_CJUMP: {
			T_stm e1 = s->u.CJUMP.left;
			T_stm e2 = s->u.CJUMP.right;
			Temp_temp r1=munchExp(e1); Temp_temp r2 = munchExp(e2);
			char *op=(char *)checked_malloc(sizeof(char)*3);
			//sprintf(assem, "cmp `s0,`s1\n");
			emit(AS_Oper("cmp `s0,`s1\n", NULL, Temp_TempList(r1, Temp_TempList(r2, NULL)), NULL));
			switch (s->u.CJUMP.op)
			{
				case T_eq:
					op = "je";
					break;
				case T_ne:
					op = "jne";
					break;
				case T_lt:
					op = "jl";
					break;
				case T_le:
					op = "jle";
					break;
				case T_gt:
					op = "jg";
					break;
				case T_ge:
					op = "jge";
					break;
				default:
					assert(0);
			}
			sprintf(assem,"%s `j0\n",op);
			emit(AS_Oper(assem, NULL, NULL, 
				AS_Targets(Temp_LabelList(s->u.CJUMP.trues,NULL))));
			//emit("cjump");
			break;
		} 
		case T_MOVE: {
			T_exp dst = s->u.MOVE.dst, src = s->u.MOVE.src;
			if (dst->kind == T_MEM) {
				if (dst->u.MEM->kind == T_BINOP&&
					dst->u.MEM->u.BINOP.op == T_plus&&
					dst->u.MEM->u.BINOP.right->kind == T_CONST) {
					/*move(mem(binop(plus,e1,const(i))),e2);*/
					T_exp e1 = dst->u.MEM->u.BINOP.left, e2 = src;
					Temp_temp r1=munchExp(e1); 
					Temp_temp r2 = munchExp(e2);
					sprintf(assem, "movl `s0,%d(`s1)\n", dst->u.MEM->u.BINOP.right->u.CONST);
					emit(AS_Move(assem, NULL,Temp_TempList(r2, Temp_TempList(r1, NULL))));
				}
				else if (dst->u.MEM->kind == T_BINOP&&
					dst->u.MEM->u.BINOP.op == T_plus&&
					dst->u.MEM->u.BINOP.left->kind == T_CONST) {
					/*move(mem(binop(plus,const(i),e1)),e2);*/
					T_exp e1 = dst->u.MEM->u.BINOP.right, e2 = src;
					Temp_temp r1 = munchExp(e1);
					Temp_temp r2 = munchExp(e2);
					sprintf(assem, "movl `s0,%d(`s1)\n", dst->u.MEM->u.BINOP.left->u.CONST);
					emit(AS_Move(assem, NULL,Temp_TempList(r2,Temp_TempList(r1, NULL))), NULL);
				}
				else if (dst->u.MEM->kind == T_CONST) {
					/*move(mem(const(i)),e2)*/
					T_exp e1 = dst->u.MEM;
					sprintf(assem, "movl `s0,($%d)\n", e1->u.CONST);
					emit(AS_Move(assem,NULL,Temp_TempList(munchExp(src),NULL)));
				}
				else if (src->kind == T_MEM) {
					/*move(mem(e1),mem(e2))*/
					T_exp e1 = dst->u.MEM, e2 = src->u.MEM;
					sprintf(assem, "movl (`s0),(`s1)\n");
					emit(AS_Move(assem, NULL,Temp_TempList(munchExp(e2),Temp_TempList(munchExp(e1),NULL))));
				}
				else {
					/*move(mem(e1),e2)*/
					T_exp e1 = dst->u.MEM, e2 = src;
					sprintf(assem, "movl `s0,(`s1)\n");
					emit(AS_Move(assem, NULL,Temp_TempList(munchExp(e2), Temp_TempList(munchExp(e1), NULL))));
				}
			}
			else if (dst->kind == T_TEMP) {
				/*move temp(e1),e2*/
				r=munchExp(src);
				sprintf(assem, "movl `s0,`d0\n");
				emit(AS_Move(assem,Temp_TempList(dst->u.TEMP,NULL),Temp_TempList(r,NULL),NULL));
			}
			else {
				r= munchExp(dst);
				Temp_temp r1 = munchExp(src);
				sprintf(assem, "movl `s0,`s1\n");
				emit(AS_Move(assem, Temp_TempList(r, NULL), Temp_TempList(r1, NULL), NULL));
			}
			break;
		}
		case T_EXP: {
			munchExp(s->u.EXP);
			break;
		}
	}
}
//push 参数入栈
static Temp_tempList munchArgs(int i, T_expList args) {
	if (!args)return NULL;
	Temp_temp tmp=munchExp(args->head);
	AS_Oper("push `s0\n",NULL,Temp_TempList(tmp,NULL),NULL);
	return Temp_TempList(tmp, munchArgs(i+1,args->tail));
}

//Maxinum Munch分析exp
static Temp_temp munchExp(T_exp s) {
	char* assem = (char*)checked_malloc(sizeof(char) * 100);
	Temp_temp r = Temp_newtemp();
	switch (s->kind)
	{
		case T_BINOP: {
			switch (s->u.BINOP.op)
			{
			case T_plus: {
				if (s->u.BINOP.left->kind == T_CONST&& s->u.BINOP.right->kind == T_CONST) {
					sprintf(assem,"movl $%d, `d0\n",s->u.BINOP.left->u.CONST+ s->u.BINOP.right->u.CONST);
					emit(AS_Oper(assem,Temp_TempList(r,NULL),NULL,NULL));
				}
				else {
					sprintf(assem, "movl `s0, `d0\n");
					emit(AS_Move(assem, Temp_TempList(r, NULL), Temp_TempList(munchExp(s->u.BINOP.left), NULL)));
					sprintf(assem, "addl `s0, `d0\n");
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(r, Temp_TempList(munchExp(s->u.BINOP.right), NULL)), NULL));
				}
				break;
			}
			case T_minus: {
				if (s->u.BINOP.left == T_CONST&& s->u.BINOP.right->kind == T_CONST) {
					sprintf(assem, "movl $%d, `d0\n", s->u.BINOP.left->u.CONST - s->u.BINOP.right->u.CONST);
					emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, NULL));
				}
				else {
					//sprintf(assem, "movl `s0, `d0\n");
					emit(AS_Move("movl `s0, `d0\n", Temp_TempList(r, NULL), Temp_TempList(munchExp(s->u.BINOP.left), NULL)));
					sprintf(assem, "subl `s0, `d0\n");
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(r, Temp_TempList(munchExp(s->u.BINOP.right), NULL)), NULL));
				}
				break;
			}
			case T_mul: {
				if (s->u.BINOP.left->kind == T_CONST&& s->u.BINOP.right->kind == T_CONST) {
					sprintf(assem, "movl $%d, `d0\n", s->u.BINOP.left->u.CONST * s->u.BINOP.right->u.CONST);
					emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, NULL));
				}
				else {
					//sprintf(assem, "movl `s0, `d0\n");
					emit(AS_Move("movl `s0, `d0\n", Temp_TempList(r, NULL), Temp_TempList(munchExp(s->u.BINOP.left), NULL)));
					sprintf(assem, "imull `s0, `d0\n");
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(r, Temp_TempList(munchExp(s->u.BINOP.right), NULL)), NULL));
				}
				break;
			}
			case T_div: {
				if (s->u.BINOP.left->kind == T_CONST&& s->u.BINOP.right->kind == T_CONST) {
					sprintf(assem, "movl $%d, `d0\n", s->u.BINOP.left->u.CONST / s->u.BINOP.right->u.CONST);
					emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, NULL));
				}
				else {
					//sprintf(assem, "movl `s0, `d0\n");
					emit(AS_Move("movl `s0, `d0\n", Temp_TempList(r, NULL), Temp_TempList(munchExp(s->u.BINOP.left), NULL)));
					sprintf(assem, "idivl `s0, `d0\n");
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(r, Temp_TempList(munchExp(s->u.BINOP.right), NULL)), NULL));
				}
				break;
			}
			default:
				break;
			}
			break;
		}
		case T_MEM: {
			if (s->u.MEM->kind == T_BINOP&&s->u.MEM->u.BINOP.op == T_plus) {
				if (s->u.MEM->u.BINOP.left->kind == T_CONST) {
					sprintf(assem, "move %d(`s0), `d0\n", s->u.MEM->u.BINOP.left->u.CONST);
					Temp_temp r1=munchExp(s->u.MEM->u.BINOP.right);
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(r1, NULL), NULL));
				}
				else if (s->u.MEM->u.BINOP.right->kind == T_CONST) {
					sprintf(assem, "move %d(`s0), `d0\n", s->u.MEM->u.BINOP.right->u.CONST);
					Temp_temp r1 = munchExp(s->u.MEM->u.BINOP.left);
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(r1, NULL), NULL));
				}
				else {
					T_exp left = s->u.MEM->u.BINOP.left;
					T_exp right = s->u.MEM->u.BINOP.right;
					Temp_temp tmp=munchExp(left);
					//sprintf(assem, "addl `s0, `d0\n");
					emit(AS_Oper("addl `s0, `d0\n", Temp_TempList(tmp, NULL),
						Temp_TempList(munchExp(right), Temp_TempList(tmp, NULL)),NULL));
					sprintf(assem, "movl (`s0), `d0\n");
					emit(AS_Move(assem, Temp_TempList(r, NULL),
						 Temp_TempList(tmp, NULL)));
				}
				break;
			}
			else if (s->u.MEM->kind == T_CONST) {
				sprintf(assem, "move (%d), `d0\n", s->u.MEM->u.CONST);
				Temp_temp r1 = munchExp(s->u.MEM->u.BINOP.left);
				emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, NULL));
			}
			else {
				Temp_temp r1 = munchExp(s->u.MEM);
				sprintf(assem, "move (`s0), `d0\n");
				emit(AS_Oper(assem,Temp_TempList(r,NULL),Temp_TempList(r1,NULL),NULL));
			}
			break;
		}
		case T_TEMP: {
			return s->u.TEMP;
		}
		case T_ESEQ: {
			munchStm(s->u.ESEQ.stm);
			return munchExp(s->u.ESEQ.exp);
		}
		case T_NAME: {
			sprintf(assem, "movl $.%s, `d0\n", Temp_labelstring(s->u.NAME));
			emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, NULL));
			return r;
		}
		case T_CONST: {
			sprintf(assem, "movl $(%d), `d0\n", s->u.CONST);
			emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, NULL));
			return r;
		}
		case T_CALL: {
			r = munchExp(s->u.CALL.fun);
			sprintf(assem, "call `s0\n");
			emit(AS_Oper(assem, F_CallerSaves(), Temp_TempList(r,munchArgs(0,s->u.CALL.args)), NULL));
			return r;
		}
		default: {
			break;
		}				 
	}
	return r;
}