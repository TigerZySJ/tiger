%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "prabsyn.h"
#include "parse.h"

int yylex(void); /* function prototype */
//int pos=1;
A_exp absyn_root;

void yyerror(char *s)
{
 EM_error(EM_tokPos, "%s", s);
}
%}


%union {
	int pos;
	int ival;
	string sval;
	float lval;
    S_symbol symbol;
    A_var var;
    A_exp exp;
    A_dec dec;
    A_ty  ty;
    A_decList decs;
    A_expList expList;
    A_field  tyfield;
    A_fieldList tyfields;
    A_fundecList funcdecs;
	A_fundec funcdec;
    A_namety tydec;
	A_nametyList tydecs;
    A_efield field;
    A_efieldList fieldlist ;
	}
%type <var> lvalue 
%type <exp> exp
%type <dec> dec
%type <decs> decs
%type <ty>  ty
%type <expList> exps 
%type <expList> expList
%type <tyfield> tyfield
%type <tyfields> tyfields
%type <funcdecs>  funcdecs
%type <funcdec>  funcdec
%type <tydec> tydec 
%type <tydecs> tydecs 
%type <field> field
%type <fieldlist>  fieldList
%type <dec>vardec
%token <sval> ID STRING
%token <ival> INT

%token 
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK 
  LBRACE RBRACE DOT 
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF 
  BREAK NIL
  FUNCTION VAR TYPE 



%left SEMICOLON
%right THEN ELSE DOT DO OF
%right ASSIGN 
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE 
%left UMINUS
%start program

%%

/* This is a skeleton grammar file, meant to illustrate what kind of
 * declarations are necessary above the %% mark.  Students are expected
 *  to replace the two dummy productions below with an actual grammar. 
 */

program:	exp		{absyn_root=$1;}

decs: dec decs		{$$=A_DecList($1,$2);}
	|dec			{$$=A_DecList($1,NULL);}
	;
dec : tydecs			{$$=A_TypeDec(EM_tokPos,$1);}
	|vardec			{$$=$1;}
	|funcdecs		{$$=A_FunctionDec(EM_tokPos,$1);}
	;
tydecs: tydec tydecs	{$$=A_NametyList($1,$2);}
	|tydec				{$$=A_NametyList($1,NULL);}
tydec: TYPE ID EQ ty	{$$=A_Namety(S_Symbol($2),$4);}
ty	:ID					{$$=A_NameTy(EM_tokPos,S_Symbol($1));}
	|LBRACE tyfields RBRACE	{$$=A_RecordTy(EM_tokPos,$2);}
    | ARRAY OF ID		{$$=A_ArrayTy(EM_tokPos,S_Symbol($3));}
	;
tyfield: ID COLON ID	{$$=A_Field(EM_tokPos,S_Symbol($1),S_Symbol($3));}
tyfields: tyfield COMMA tyfields	{$$=A_FieldList($1,$3);}
		|tyfield		{$$=A_FieldList($1,NULL);}
		;
vardec: VAR ID ASSIGN exp	{$$=A_VarDec(EM_tokPos, S_Symbol($2),NULL, $4);}
	|VAR ID COLON ID ASSIGN exp	{$$=A_VarDec(EM_tokPos, S_Symbol($2),S_Symbol($4), $6);}
	;
funcdecs:funcdec			{$$=A_FundecList($1,NULL);}
	|funcdec funcdecs	{$$=A_FundecList($1,$2);}
	;
funcdec: FUNCTION ID LPAREN tyfields RPAREN EQ exp	{$$=A_Fundec(EM_tokPos, S_Symbol($2), $4, NULL,$7);}
	|  FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp	{$$=A_Fundec(EM_tokPos, S_Symbol($2), $4, S_Symbol($7),$9);}
	|FUNCTION ID LPAREN RPAREN EQ exp		{$$=A_Fundec(EM_tokPos, S_Symbol($2), NULL, NULL,$6);}
	|FUNCTION ID LPAREN RPAREN COLON ID EQ exp	{$$=A_Fundec(EM_tokPos, S_Symbol($2), NULL, S_Symbol($6),$8);}
	;
lvalue: ID				{$$=A_SimpleVar(EM_tokPos,S_Symbol($1));}
	|ID LBRACK exp RBRACK	{$$=A_SubscriptVar(EM_tokPos,A_SimpleVar(EM_tokPos,S_Symbol($1)),$3)}
	|lvalue DOT ID			{$$=A_FieldVar(EM_tokPos,$1,S_Symbol($3));}
	|lvalue LBRACK exp RBRACK	{$$=A_SubscriptVar(EM_tokPos,$1,$3)}
	;
exps : exp SEMICOLON exps	{$$=A_ExpList($1, $3);}
	| exp					{$$=A_ExpList($1, NULL);}
	;
expList: exp				{$$=A_ExpList($1, NULL);}
	|exp COMMA expList		{$$=A_ExpList($1, $3);}
	;
field: ID EQ exp			{$$=A_Efield(S_Symbol($1), $3);}
fieldList:field				{$$=A_EfieldList($1, NULL);}
	|field COMMA fieldList	{$$=A_EfieldList($1,$3);}
exp: LPAREN exps RPAREN		{$$=A_SeqExp(EM_tokPos,$2);}
	|lvalue					{$$=A_VarExp(EM_tokPos,$1);}
	|NIL					{$$=A_NilExp(EM_tokPos);}
	|INT					{$$=A_IntExp(EM_tokPos,$1);}
	|STRING					{$$=A_StringExp(EM_tokPos,$1);}
	|LPAREN RPAREN			{$$=A_NilExp(EM_tokPos)}
	//call function
	|ID LPAREN RPAREN		{$$=A_CallExp(EM_tokPos, S_Symbol($1),NULL);}
	|ID LPAREN expList RPAREN	{$$=A_CallExp(EM_tokPos, S_Symbol($1),$3);}
	
	//exp1 op exp2
	|MINUS exp %prec UMINUS	{$$=A_OpExp(EM_tokPos, A_minusOp,A_IntExp(EM_tokPos, 0) ,$2);}
	|exp PLUS exp		{$$=A_OpExp(EM_tokPos, A_plusOp,$1 ,$3);}
	|exp MINUS exp		{$$=A_OpExp(EM_tokPos, A_minusOp,$1 ,$3);}
	|exp TIMES exp		{$$=A_OpExp(EM_tokPos, A_timesOp,$1 ,$3);}
	|exp DIVIDE exp		{$$=A_OpExp(EM_tokPos, A_divideOp,$1 ,$3);}
	|exp EQ exp			{$$=A_OpExp(EM_tokPos, A_eqOp,$1 ,$3);}
	|exp NEQ exp		{$$=A_OpExp(EM_tokPos, A_neqOp,$1 ,$3);}
	|exp LT exp			{$$=A_OpExp(EM_tokPos, A_ltOp,$1 ,$3);}
	|exp LE exp			{$$=A_OpExp(EM_tokPos, A_leOp,$1 ,$3);}
	|exp GT exp			{$$=A_OpExp(EM_tokPos, A_gtOp,$1 ,$3);}
	|exp GE exp			{$$=A_OpExp(EM_tokPos, A_geOp,$1 ,$3);}
	|exp AND exp		{$$=A_IfExp(EM_tokPos,$1,$3,A_IntExp(EM_tokPos ,0)); }
	|exp OR exp			{$$=A_IfExp(EM_tokPos,$1,A_IntExp(EM_tokPos,1),$3 ); }
	
	//record
	|ID LBRACE RBRACE	{$$=A_RecordExp(EM_tokPos, S_Symbol($1), NULL);}
	|ID LBRACE fieldList RBRACE	{$$=A_RecordExp(EM_tokPos,S_Symbol($1) , $3);} 
	
	//array
	|ID LBRACK exp RBRACK OF exp	{$$=A_ArrayExp(EM_tokPos, S_Symbol($1),$3,$6);}
	
	//assign
	|lvalue ASSIGN exp	{ $$ = A_AssignExp(EM_tokPos , $1 , $3) ; }
	
	//if-else-then
	|IF exp THEN exp ELSE exp	{$$=A_IfExp(EM_tokPos,$2, $4, $6);}
	|IF exp THEN exp			{$$=A_IfExp(EM_tokPos,$2, $4, NULL);}
	
	//while
	|WHILE exp DO exp			{$$=A_WhileExp(EM_tokPos, $2, $4);}
	
	//for
	|FOR ID ASSIGN exp TO exp DO exp	{$$=A_ForExp(EM_tokPos, S_Symbol($2), $4,$6,$8);}
	
	//break
	|BREAK						{$$=A_BreakExp(EM_tokPos);}
	
	//let
	|LET decs IN exps END		{$$=A_LetExp(EM_tokPos, $2,A_SeqExp( EM_tokPos , $4 ));}
	|LET decs IN END			{$$=A_LetExp(EM_tokPos, $2,NULL);}
%%
string toknames[] = {
"ID", "STRING", "INT", "COMMA", "COLON", "SEMICOLON", "LPAREN",
"RPAREN", "LBRACK", "RBRACK", "LBRACE", "RBRACE", "DOT", "PLUS",
"MINUS", "TIMES", "DIVIDE", "EQ", "NEQ", "LT", "LE", "GT", "GE",
"AND", "OR", "ASSIGN", "ARRAY", "IF", "THEN", "ELSE", "WHILE", "FOR",
"TO", "DO", "LET", "IN", "END", "OF", "BREAK", "NIL", "FUNCTION",
"VAR", "TYPE","CONST_INT","CONST_STRING","CONST_FLOAT"
};


string tokname(tok) {
  return tok<257 || tok>301 ? "BAD_TOKEN" : toknames[tok-258];
}
/*
int main(int argc, char **argv){
 string fname; int tok;
 if (argc!=3) {fprintf(stderr,"usage: a.out input_filename output_filename\n"); exit(1);}
fname=argv[1];
 EM_reset(fname);
for(;;) {
   tok=yylex();
   if (tok==0) break;
   switch(tok) {
   case ID: case STRING:
     printf("%10s %4d %s\n",tokname(tok),EM_tokPos,yylval.sval);
     break;
   case INT:
     printf("%10s %4d %d\n",tokname(tok),EM_tokPos,yylval.ival);
     break;
   default:
     printf("%10s %4d\n",tokname(tok),EM_tokPos);
   }
 }
absyn_root=parse(fname);
string fname2 =argv[2];
FILE*out=fopen(fname2,"w");
if(absyn_root!=NULL){
	printf("The root is not NULL\n");
	pr_exp(out,absyn_root,4);
}
else 
	printf("The root is NULL\n");
fclose(out);

}*/