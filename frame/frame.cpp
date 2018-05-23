#include "frame.h"
F_accesslist F_Accesslist(F_access head, F_accesslist tail)
{
	F_accesslist ptr = (F_accesslist)checked_malloc(sizeof(*ptr));
	ptr->head = head;
	ptr->tail = tail;
	return ptr;
}
F_access InFrame(int offset) {
	F_access ptr = (F_access)checked_malloc(sizeof(*ptr));
	ptr->kind = F_access_::inFrame;
	ptr->u.offset = offset;
	return ptr;
}
F_access InReg(Temp_temp reg)
{
	F_access ptr = (F_access)checked_malloc(sizeof(*ptr));
	ptr->kind = F_access_::inReg;
	ptr->u.reg = reg;
	return ptr;
}
F_frame F_newframe(Temp_label name, U_boolList formals) {
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
		frame->formals = F_Accesslist(access, frame->formals);
		formals = formals->tail;
	}
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
F_accesslist F_formals(F_frame f) {
	return f->formals;
}
T_stm F_procEntryExit1(F_frame frame, T_stm stm)
{
	return stm;
}