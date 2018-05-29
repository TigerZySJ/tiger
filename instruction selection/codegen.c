#include "codegen.h"
static AS_instrList assem_instr = NULL;
//利用Maxinum Munch算法产生assem算法
AS_instrList F_codegen(F_frame frame, T_stmList stmList) {
	T_stmList tmp = NULL;
	for (tmp = stmList; tmp->head; tmp = tmp->tail) {
		munchStm(tmp->head);
	}
	return assem_instr;
}
//添加assem指令
static void emit(AS_instr instr) {
	if (!assem_instr) {
		assem_instr->head = AS_instrList(instr,NULL);
	}
	else {
		assem_instr->tail = AS_instrList(instr, NULL);
	}
}
//Maxinum Munch分析stm
static void munchStm(T_stm s) {
	char assem[100];
	Temp_temp r = Temp_newtemp();
	switch (s->kind) {
		case T_SEQ: {
			T_stm e1 = s->u.SEQ.left;
			T_stm e2 = s->u.SEQ.right;
			munchExp(e1); munchExp(e2);
			break;
		} 
		case T_LABEL: {
			sprintf(assem,"%s",Temp_labelstring(s->u.LABEL));
			emit(AS_Label(string(assem), s->u.LABEL));
			break;
		}
		case T_JUMP: {
			Temp_temp r = munchExp(s->u.JUMP.exp);
			emit(AS_Oper(string("jmp `d0"), Temp_TempList(r, NULL), NULL, AS_Targets(s->u.JUMP.jumps)));
			//emit("jump");
			break;
		}
		case T_CJUMP: {
			T_stm e1 = s->u.CJUMP.true;
			T_stm e2 = s->u.CJUMP.false;
			T_exp r1=munchExp(e1); T_exp r2 = munchExp(e2);
			char op[3];
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
			sprintf(assem,"%s %s",op,Temp_labelstring(s->u.CJUMP.true));
			emit(AS_Oper(assem, NULL, NULL, AS_Targets(s->u.JUMP.jumps)));
			//emit("cjump");
			break;
		} 
		case T_MOVE: {
			T_exp dst = s->u.MOVE.dst, src = s->u.MOVE.src;
			if (dst->kind == T_MEM) {
				if (dst->u.MEM->kind == T_BINOP&&
					dst->u.MEM->u.BINOP.op == T_PLUS&&
					dst->u.MEM->u.BINOP.right->kind == T_CONST) {
					/*move(mem(binop(plus,e1,const(i))),e2);*/
					T_exp e1 = dst->u.MEM->u.BINOP.left, e2 = src->u.MEM->u.BINOP.right;
					Temp_temp r1=munchExp(e1); 
					sprintf(assem, "movl %d(`s1),`s0"), e2->u.CONST);
					emit(AS_Move(assem));
				}
				else if (dst->u.MEM->kind == T_BINOP&&
					dst->u.MEM->u.BINOP.op == T_PLUS&&
					dst->u.MEM->u.BINOP.left->kind == T_CONST) {
					/*move(mem(binop(plus,const(i),e1)),e2);*/
					T_exp e1 = dst->u.MEM->u.BINOP.left, e2 = src->u.MEM->u.BINOP.right;
					Temp_temp r1 = munchExp(e2);
					sprintf(assem, "movl %d(`s0),`d0"), e1->u.CONST);
					emit(AS_Move(assem, Temp_TempList(r, NULL), Temp_TempList(r2, NULL)), NULL);
				}
				else if (dst->u.MEM->kind == T_CONST) {
					/*move(mem(const(i)),e2)*/
					T_exp e1 = dst->u.MEM;
					sprintf(assem, "movl `s0,%d"), e1->u.CONST);
					emit(AS_Move(assem,NULL,Temp_TempList(munchExp(src),NULL)));
				}
				else if (src->kind == T_MEM) {
					/*move(mem(e1),mem(e2))*/
					T_exp e1 = dst->u.MEM, e2 = src->u.MEM;
					sprintf(assem, "movl `s0,`s1"));
					emit(assem, Temp_TempList(munchExp(e1),NULL), Temp_TempList(munchExp(e2),NULL));
				}
				else {
					/*move(mem(e1),e2)*/
					T_exp e1 = dst->u.MEM, e2 = src;
					sprintf(assem, "movl `s0,`s1"));
					emit(assem, Temp_TempList(munchExp(e1), NULL), Temp_TempList(munchExp(e2), NULL));
				}
			}
			else if (dst->kind == T_TEMP) {
				/*move temp(e1),e2*/
				r=munchExp(src);
				sprintf(assem, "movl `s0,`s1"));
				emit(assem,Temp_TempList(dst->u.TEMP,NULL),Temp_TempList(r,NULL),NULL);
			}
			else {
				r= munchExp(dst);
				Temp_temp r1 = munchExp(src);
				sprintf(assem, "movl `s0,`s1"));
				emit(assem, Temp_TempList(r, NULL), Temp_TempList(r1, NULL), NULL);
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
	AS_Oper("push `s0\n",NULL,Temp_TempList(,NULL),NULL);
	return Temp_tempList(tmp, munchArgs(i+1,args->tail));
}

//Maxinum Munch分析exp
static Temp_temp munchExp(T_exp s) {
	char assem[100];
	Temp_temp r = Temp_newtemp();
	switch (s->kind)
	{
		case T_BINOP: {
			switch (s->u.BINOP.op)
			{
			case T_plus: {
				if (s->u.BINOP.left == CONST&& s->u.BINOP.right == CONST) {
					sprintf(assem,"movl $(%d), `d0\n",s->u.BINOP.left->u.CONST+ s->u.BINOP.right->u.CONST);
					emit(AS_Oper(assem,Temp_TempList(r,NULL),NULL,NULL));
				}
				else {
					sprintf(assem, "movl `s0, `d0\n");
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(munchExp(s->u.BINOP.left),NULL), NULL));
					sprintf(assem, "addl `s0, `d0\n");
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(r, Temp_TempList(munchExp(s->u.BINOP.right), NULL)), NULL));
				}
				return e1;
			}
			case T_minus: {
				if (s->u.BINOP.left == CONST&& s->u.BINOP.right == CONST) {
					sprintf(assem, "movl $(%d), `d0\n", s->u.BINOP.left->u.CONST - s->u.BINOP.right->u.CONST);
					emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, NULL));
				}
				else {
					sprintf(assem, "movl `s0, `d0\n");
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(munchExp(s->u.BINOP.left), NULL), NULL));
					sprintf(assem, "subl `s0, `d0\n");
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(r, Temp_TempList(munchExp(s->u.BINOP.right), NULL)), NULL));
				}
			}
			case T_mul: {
				if (s->u.BINOP.left == CONST&& s->u.BINOP.right == CONST) {
					sprintf(assem, "movl $(%d), `d0\n", s->u.BINOP.left->u.CONST * s->u.BINOP.right->u.CONST);
					emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, NULL));
				}
				else {
					sprintf(assem, "movl `s0, `d0\n");
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(munchExp(s->u.BINOP.left), NULL), NULL));
					sprintf(assem, "imull `s0, `d0\n");
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(r, Temp_TempList(munchExp(s->u.BINOP.right), NULL)), NULL));
				}
			}
			case T_div: {
				if (s->u.BINOP.left == CONST&& s->u.BINOP.right == CONST) {
					sprintf(assem, "movl $(%d), `d0\n", s->u.BINOP.left->u.CONST / s->u.BINOP.right->u.CONST);
					emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, NULL));
				}
				else {
					sprintf(assem, "movl `s0, `d0\n");
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(munchExp(s->u.BINOP.left), NULL), NULL));
					sprintf(assem, "idivl `s0, `d0\n");
					emit(AS_Oper(assem, Temp_TempList(r, NULL), Temp_TempList(r, Temp_TempList(munchExp(s->u.BINOP.right), NULL)), NULL));
				}
			}
			default:
				break;
			}
			
		}
		case T_MEM: {
			if (s->u.MEM->kind == T_BINOP&&s->u.MEM->u.BINOP == T_plus) {
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
			}
			else if (s->u.MEM->kind == T_CONST) {
				sprintf(assem, "move 0x%d, `d0\n", s->u.MEM->u.CONST);
				Temp_temp r1 = munchExp(s->u.MEM->u.BINOP.left);
				emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, NULL));
			}
			else {
				Temp_temp r1 = munchExp(s->u.MEM->u);
				sprintf(assem, "move `s0, `d0\n");
				emit(AS_Oper(assem,Temp_TempList(r,NULL),Temp_TempList(r1,NULL),NULL));
			}
		}
		case T_TEMP: {
			return s->u.TEMP;
		}
		case T_ESEQ: {
			munchStm(s->u.ESEQ.stm);
			return munchExp(s->u.ESEQ.exp);
		}
		case T_NAME: {
			sprintf(assem, "movl $(%s), `d0\n", S_name(s->u.NAME));
			emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, NULL));
			return r;
		}
		case T_CONST: {
			sprintf(assem, "movl $(%d), `d0\n", s->u.CONST);
			emit(AS_Oper(assem, Temp_TempList(r, NULL), NULL, NULL));
			return r;
		}
		case T_CALL: {
			sprintf(assem, "call `s0\n");
			emit(AS_Oper(assem, Temp_TempList(r, NULL), munchArgs(0,s->u.CALL.args), NULL));
			return r;
		}
		default: {
			break;
		}
	}
}