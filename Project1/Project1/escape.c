#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "table.h"
#include "escape.h"
typedef struct Esc_escapeEntry_ *Esc_escapeEntry;
struct Esc_escapeEntry_ {
	int depth;
	bool *escape;
};
Esc_escapeEntry Esc_EscapeEntry(int depth, bool* escape) {
	Esc_escapeEntry p = (Esc_escapeEntry)checked_malloc(sizeof(*p));
	p->depth = depth;
	p->escape = escape;
	return p;
}
static void traverseExp(S_table env, int depth, A_exp e);
static void traverseDec(S_table env, int depth, A_dec d);
static void traverseVar(S_table env, int depth, A_var v);

void Esc_findEscape(A_exp exp) {
	S_table escapeTable = S_empty();
	traverseExp(escapeTable, 1, exp);
}
static void traverseExp(S_table env, int depth, A_exp e) {
	switch (e->kind)
	{

	case A_nilExp:
	case A_intExp:
	case A_stringExp:
	case A_breakExp:
		break;
	case A_varExp: {
		traverseVar(env, depth, e->u.var);
		break;
	}
	case A_callExp:{
		A_expList explist = e->u.call.args;
		for (; explist; explist = explist->tail)
			traverseExp(env, depth, explist->head);
		break;
	}
	case A_opExp: {
		traverseExp(env, depth, e->u.op.left);
		traverseExp(env, depth, e->u.op.right);
		break;
	}
	case A_seqExp: {
		A_expList explist = e->u.seq;
		for (; explist; explist = explist->tail)
			traverseExp(env, depth, explist->head);
		break;
	}
	case A_assignExp: {
		traverseVar(env, depth, e->u.assign.var);
		traverseExp(env, depth, e->u.assign.exp);
		break;
	}
	case A_recordExp: {
		A_efieldList efiledList = e->u.record.fields;
		for (; efiledList; efiledList = efiledList->tail)
			traverseExp(env, depth, efiledList->head->exp);
		break;
	}
	case A_whileExp: {
		traverseExp(env, depth, e->u.whilee.test);
		traverseExp(env, depth, e->u.whilee.body);
		break;
	}
	case A_ifExp: {
		traverseExp(env, depth, e->u.iff.test);
		traverseExp(env, depth, e->u.iff.then);
		if (e->u.iff.elsee)
			traverseExp(env, depth, e->u.iff.elsee);
		break;
	}
	case A_forExp: {
		traverseExp(env, depth, e->u.forr.lo);
		traverseExp(env, depth, e->u.forr.hi);
		S_beginScope(env);
		e->u.forr.escape = FALSE;
		S_enter(env, e->u.forr.var, Esc_EscapeEntry(depth, &(e->u.forr.escape)));
		traverseExp(env, depth, e->u.forr.body);
		S_endScope(env);
		break;
	}
	case A_letExp: {
		S_beginScope(env);
		A_decList decList = e->u.let.decs;
		for (; decList; decList = decList->tail)
			traverseDec(env, depth, decList->head);
		traverseExp(env, depth, e->u.let.body);
		S_endScope(env);
		break;
	}
	case A_arrayExp: {
		traverseExp(env, depth, e->u.array.size);
		traverseExp(env, depth, e->u.array.init);
		break;
	}
	default:
		break;
	}
}
static void traverseDec(S_table env, int depth, A_dec d) {
	switch (d->kind)
	{
	case A_typeDec:
		break;
	case A_varDec: {
		traverseExp(env, depth, d->u.var.init);
		d->u.var.escape = FALSE;
		S_enter(env, d->u.var.var, Esc_EscapeEntry(depth, &(d->u.var.escape)));
		break;
	}
	case A_functionDec: {
		A_fundecList fundecList=d->u.function;
		for (; fundecList; fundecList = fundecList->tail) {
			A_fundec f = fundecList->head;
			S_beginScope(env);
			A_fieldList l=f->params;
			for (; l; l = l->tail) {
				l->head->escape = FALSE;
				S_enter(env, l->head->name, Esc_EscapeEntry(depth + 1, &(l->head->escape)));
			}
			traverseExp(env, depth + 1, f->body);
			S_endScope(env);
		}
		break;
	}
	
	default: {
		assert(0);
		break;
	}
	}
}
static void traverseVar(S_table env, int depth, A_var v) {
	switch (v->kind)
	{
	case A_fieldVar: {
		traverseVar(env, depth, v->u.field.var);
		break;
	}
	case A_simpleVar: {
		Esc_escapeEntry entry = S_look(env, v->u.simple);
		if (entry) {
			if (depth > entry->depth)
				*(entry->escape) = true;
		}
		break;
	}
	case A_subscriptVar: {
		traverseVar(env, depth, v->u.subscript.var);
		traverseExp(env, depth, v->u.subscript.exp);
		break;
	}
	default: {
		assert(0);
		break;
	}
	}
}
