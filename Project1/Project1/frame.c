#include "frame.h"

F_accessList F_Accesslist(F_access head, F_accessList tail)
{
	F_accessList ptr = (F_accessList)checked_malloc(sizeof(*ptr));
	ptr->head = head;
	ptr->tail = tail;
	return ptr;
}
F_access InFrame(int offset) {
	F_access ptr = (F_access)checked_malloc(sizeof(*ptr));
	ptr->kind = inFrame;
	ptr->u.offset = offset;
	return ptr;
}
F_access InReg(Temp_temp reg)
{
	F_access ptr = (F_access)checked_malloc(sizeof(*ptr));
	ptr->kind = inReg;
	ptr->u.reg = reg;
	return ptr;
}
F_frame F_newFrame(Temp_label name, U_boolList formals) {
	F_frame newframe=(F_frame)checked_malloc(sizeof(*newframe));
	newframe->function_address = name;
	newframe->formals = NULL;
	newframe->locals = NULL;
	newframe->framesize = 0;
	while (formals!=NULL) {
		F_access access = (F_access)checked_malloc(sizeof(*access));
		if (formals->head) {
			//如果是逃逸变量，则在内存中
			access = InFrame(newframe->framesize);//小端模式
			newframe->framesize += offset;
		}
		else {
			//如果不是逃逸变量，则放在寄存器中
			access = InReg(Temp_newtemp());
		}
		newframe->formals = F_Accesslist(access, newframe->formals);
		formals = formals->tail;
	}
	return newframe;
}
F_fragList F_FragList(F_frag frag, F_fragList tail) {
	F_fragList ptr = (F_fragList)checked_malloc(sizeof(*ptr));
	ptr->head = frag;
	ptr->tail = tail;
	return ptr;
}
Temp_label F_name(F_frame f) {
	return f->function_address;
}
F_accessList F_formals(F_frame f) {
	return f->formals;
}
F_access F_allocLocal(F_frame f, bool escape) {
	F_access access = (F_access)checked_malloc(sizeof(*access));
	if(escape==true){
		access = InFrame(f->framesize);
		f->framesize += offset;
	}
	else {
		access = InReg(Temp_newtemp());
	}
	f->locals = F_Accesslist(access, f->locals);
	return access;
}
F_frag F_StringFrag(Temp_label label, string str) {
	F_frag frag = (F_frag)checked_malloc(sizeof(*frag));
	frag->kind = F_stringFrag;
	frag->u.stringg.str = str;
	frag->u.stringg.label = label;
	return frag;
}
F_frag F_ProcFrag(T_stm stm, F_frame frame) {
	F_frag frag = (F_frag)checked_malloc(sizeof(*frag));
	frag->kind = F_procFrag;
	frag->u.proc.frame = frame;
	frag->u.proc.body=stm;
	return frag;
}

T_exp F_Exp(F_access acc, T_exp framePtr) {
	if (acc->kind == inFrame)
		return T_Mem(T_Binop(T_plus,framePtr, T_Const(acc->u.offset)));
	return T_Temp(acc->u.reg);
}
T_exp F_externalCall(string s, T_expList explist) {
	return T_Call(T_Name(Temp_namedlabel(s)), explist);
}
Temp_temp F_FP(void) {
	if (!fp)
		fp = Temp_newtemp();
	return fp;
}
Temp_temp F_RV()
{
	if (!rv)
		rv = Temp_newtemp();
	return rv;
}
Temp_temp F_SP(void) {
	if (!sp)
		sp = Temp_newtemp();
	return sp;
}
Temp_temp F_DX(void) {
	if (!dx)
		dx = Temp_newtemp();
	return dx;
}
Temp_temp F_CX(void) {
	if (!cx)
		cx = Temp_newtemp();
	return cx;
}
Temp_temp F_BX(void) {
	if (!bx)
		bx = Temp_newtemp();
	return bx;
}
Temp_temp F_DI(void) {
	if (!di)
		di = Temp_newtemp();
	return di;
}
Temp_temp F_SI(void) {
	if (!si)
		si = Temp_newtemp();
	return si;
}
Temp_map F_TempMap() {
	static Temp_map initial = NULL;
	if (initial == NULL) {
		initial = Temp_empty();
		Temp_enter(initial, F_SP(), "%esp");
		Temp_enter(initial, F_FP(), "%ebp");
		Temp_enter(initial, F_RV(), "%eax");
		Temp_enter(initial, F_DX(), "%edx");
		Temp_enter(initial, F_CX(), "%ecx");
		Temp_enter(initial, F_BX(), "%ebx");
		Temp_enter(initial, F_DI(), "%edi");
		Temp_enter(initial, F_SI(), "%esi");
	}
	return initial;

}
Temp_tempList F_specialregs() {
	static Temp_tempList specialregs = NULL;
	if (!specialregs) {
		specialregs = Temp_TempList(F_SP(), Temp_TempList(F_FP(), NULL));
	}
	return specialregs;
}
Temp_tempList F_CallerSaves() {
	if (!callersaves)
		callersaves = Temp_TempList(F_RV(), Temp_TempList(F_CX(), Temp_TempList(F_DX(), NULL)));
	return callersaves;
}
Temp_tempList F_CalleeSaves() {
	if (!calleesaves)
		calleesaves = Temp_TempList(F_BX(), Temp_TempList(F_SI(), Temp_TempList(F_DI(), NULL)));
	return calleesaves;
}
T_stm F_procEntryExit1(F_frame frame, T_stm stm)
{
	T_stm ptr=T_Seq(T_Exp(T_Const(0)), NULL);
	T_stm begin = ptr;
	T_stm after = T_Exp(T_Const(0));
	Temp_tempList calleesaves = F_CalleeSaves();
	Temp_tempList tmplist=calleesaves;
	for (; tmplist; tmplist = tmplist->tail) {
		F_access local = F_allocLocal(frame,false);
		T_stm save = T_Seq(T_Move(F_Exp(local,NULL),T_Temp(tmplist->head)),NULL);
		begin->u.SEQ.right = save;
		begin = save;
		after= T_Seq(T_Move(T_Temp(tmplist->head), F_Exp(local, NULL)),after);
	}
	begin->u.SEQ.right = T_Seq(stm, after);
	return ptr;
}
AS_instrList F_procEntryExit2(AS_instrList body) {    /////
	static Temp_tempList returnSink = NULL;
	if (!returnSink) {
		returnSink = Temp_TempList(F_SP(), Temp_TempList(F_FP(), F_CalleeSaves()));
	}
	return AS_splice(body, AS_InstrList(AS_Oper("", NULL, returnSink, NULL), NULL));    // only for liveness analysis; it will be deleted in regalloc.c :: deleteInstrs
}
AS_proc F_procEntryExit3(F_frame frame, AS_instrList body) {
	AS_instrList pro = NULL;
	AS_instrList epi = NULL;
	AS_instrList result;
	char* instr = checked_malloc(50);
	F_accessList locals=frame->locals;
	int localCount = 0;
	for (; locals; locals = locals->tail)
		localCount++;
	sprintf(instr, "subl $%d,`d0", 4 * localCount);
	pro = AS_InstrList(AS_Oper(instr,Temp_TempList(F_SP(),NULL),NULL,NULL),NULL);
	pro = AS_InstrList(AS_Oper("movl `s0,`d0", Temp_TempList(F_FP(), NULL), Temp_TempList(F_SP(), NULL), NULL), pro);
	pro = AS_InstrList(AS_Oper(instr, Temp_TempList(F_FP(), NULL), NULL, NULL), NULL);

	epi = AS_InstrList(AS_Oper("ret",NULL,NULL,NULL),NULL);
	epi = AS_InstrList(AS_Oper("leave", NULL, NULL, NULL), epi);

	result = AS_splice(pro, AS_splice(body, epi));
	return result;
}