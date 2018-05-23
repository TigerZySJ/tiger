#pragma once
#ifndef  FRAME_H_
#define FRAME_H_
#include "util.h"
#include "temp.h"
#include <string.h>

using namespace std;
typedef struct F_frame_ * F_frame;
typedef struct F_access_ * F_access;
typedef struct F_accessList_ * F_accessList;
typedef struct F_frag_ *F_frag;
typedef struct F_fragList_ * F_fragList;
const int F_wordSize = 4;
const int offset = 4;
struct F_frame_ {
	int framesize;//栈的大小
	Temp_label function_address;//栈的地址
	F_accessList formals;//传递参数
	F_accessList locals;//局部变量

};
struct F_frag_ {
	enum{F_StringFrag,F_procFrag}kind;
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

struct F_fragList_ { F_frag head; F_fragList tail; };
F_frame F_newframe(Temp_label name,U_boolList formals);
F_accesslist F_Accesslist(F_access head, F_accesslist tail);
F_accesslist F_formals(F_frame f);
F_access F_allocLocal(F_frame f, bool escape);
F_access InFrame(int offset);
F_access InReg(Temp_temp reg);
F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm stm, F_frame frame);
F_fragList F_FragList(F_frag frag, F_fragList tail);
T_exp F_externalCall(string s, T_expList explist);
Temp_temp F_RV(void);
Temp_temp F_FP(void);
Temp_label F_name(F_frame f);
T_stm F_procEntryExit1(F_frame frame, T_stm stm);
T_exp F_Exp(F_access acc, T_exp framePtr);
#endif // ! FRAME_H_
