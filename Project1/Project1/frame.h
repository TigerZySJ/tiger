#pragma once
#ifndef  FRAME_H
#define FRAME_H
#include "util.h"
#include "temp.h"
#include "tree.h"
#include "symbol.h"
#include "assem.h"
typedef struct F_frame_ * F_frame;
typedef struct F_access_ * F_access;
typedef struct F_accessList_ * F_accessList;
typedef struct F_frag_ *F_frag;
typedef struct F_fragList_ * F_fragList;
static const int F_wordSize = 4;
static const int offset = 4;
static Temp_temp fp = NULL;
static Temp_temp rv = NULL;
static Temp_temp sp = NULL;
static Temp_temp dx = NULL;
static Temp_temp cx = NULL;
static Temp_temp bx = NULL;
static Temp_temp di = NULL;
static Temp_temp si = NULL;
static Temp_tempList callersaves = NULL;
static Temp_tempList calleesaves = NULL;

struct F_frame_ {
	int framesize;//栈的大小
	Temp_label function_address;//栈的地址
	F_accessList formals;//传递参数
	F_accessList locals;//局部变量
};
struct F_frag_ {
	enum { F_stringFrag, F_procFrag }kind;
	union {
		struct
		{
			Temp_label label;
			string str;
		}stringg;
		struct {
			T_stm body;
			F_frame frame;
		}proc;
	}u;
};

struct F_access_ {
	enum { inFrame, inReg } kind;
	union {
		int offset;
		Temp_temp reg;
	} u;
};
struct F_accessList_ {
	F_access head;
	F_accessList tail;
};
extern Temp_map F_tempMap;
struct F_fragList_ { F_frag head; F_fragList tail; };
F_frame F_newFrame(Temp_label name,U_boolList formals);
F_accessList F_Accesslist(F_access head, F_accessList tail);
F_accessList F_formals(F_frame f);
F_access F_allocLocal(F_frame f, bool escape);
F_access InFrame(int offset);
F_access InReg(Temp_temp reg);
F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm stm, F_frame frame);
F_fragList F_FragList(F_frag frag, F_fragList tail);
T_exp F_externalCall(string s, T_expList explist);
Temp_temp F_RV(void);
Temp_temp F_FP(void);
Temp_temp F_SP(void);
Temp_temp F_DX(void);
Temp_temp F_CX(void);
Temp_temp F_BX(void);
Temp_temp F_SP(void);
Temp_temp F_DI(void);
Temp_temp F_SI(void);
Temp_tempList F_specialregs();
Temp_label F_name(F_frame f);
Temp_tempList F_CallerSaves();
Temp_tempList F_CalleeSaves();
T_stm F_procEntryExit1(F_frame frame, T_stm stm);
AS_instrList F_procEntryExit2(AS_instrList body);
AS_proc F_procEntryExit3(F_frame frame, AS_instrList body);
T_exp F_Exp(F_access acc, T_exp framePtr);
#endif // ! FRAME_H_
