#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "semant.h"
#include "canon.h"
#include "translate.h"
#include "assem.h"
#include "codegen.h"
#include "escape.h"
extern A_exp absyn_root;
Temp_map F_tempMap;
static void Proc_print(FILE*out, F_frame frame,T_stm body){
	F_tempMap = F_TempMap();
	T_stmList stmList = C_linearize(body);
	struct C_block block = C_basicBlocks(stmList);
	stmList = C_traceSchedule(block);
	AS_instrList instr_list = F_codegen(frame, stmList);
	fprintf(out, "START %s\n", Temp_labelstring(F_name(frame)));
	AS_printInstrList(out,instr_list,Temp_layerMap(F_tempMap, Temp_name()));
	fprintf(out, "END %s\n", Temp_labelstring(F_name(frame)));
}
static void Str_print(FILE*out,Temp_label label,string str) {
	fprintf(out, "%s:", S_name(label));
	fprintf(out,"%s\n",str);
}
int main(int argc, char **argv) {
	string fname = (string)checked_malloc(100);
	int tok;
	//if (argc != 3) { fprintf(stderr, "usage: a.out input_filename output_filename\n"); exit(1); }
	scanf("%s", fname);
	string target = (string)checked_malloc(100);

	absyn_root = parse(fname);
	if (absyn_root != NULL) {
		printf("The root is not NULL\n");
		scanf("%s", target);
		FILE*out = fopen(target, "w");
		pr_exp(out, absyn_root, 4);
		fclose(out);
		Esc_findEscape(absyn_root);
		S_table venv = E_base_venv();
		S_table tenv = E_base_tenv();
		Tr_level newLevel = Tr_newLevel(Tr_outermost(), Temp_namedlabel("main"), NULL);
		struct expty bodyTy = transExp(venv, tenv, absyn_root, newLevel, NULL);
		Tr_procEntryExit(newLevel, Tr_funDec(Tr_ExpList(bodyTy.exp,NULL)), NULL);
		F_fragList frags = Tr_getResult();
		printf("there is no error in type check\n");
		scanf("%s", target);
		out = fopen(target, "w");
		//T_stmList stmList = T_StmList(unNx(bodyTy.exp), NULL);
		for (; frags; frags = frags->tail) {
			if (frags->head->kind == F_procFrag) {
				T_stmList stmList = T_StmList(frags->head->u.proc.body, NULL);
				printStmList(out, stmList);
			}
			else
				Str_print(out, frags->head->u.stringg.label,
					frags->head->u.stringg.str);
		}
		fclose(out);
		scanf("%s", target);
		out = fopen(target, "w");
		frags = Tr_getResult();
		for (; frags; frags = frags->tail) {
			if (frags->head->kind == F_procFrag)
				Proc_print(out,frags->head->u.proc.frame,
					frags->head->u.proc.body);
			else
				Str_print(out,frags->head->u.stringg.label,
					frags->head->u.stringg.str);
		}
		fclose(out);
		//AS_instrList instr_list=F_codegen(frame, stmList);
		//AS_printInstrList(out, instr_list, Temp_map m);
		//fclose(out);
	}
	else
		printf("The root is NULL\n");



	system("pause");
}