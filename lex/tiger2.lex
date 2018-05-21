%{
#include <string.h>
#include "util.h"
#include "tokens.h"
#include "errormsg.h"
#include <stdio.h>
#include <stdlib.h>
int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}


void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}
int comment_count=0;
string str;
void init(){
	str=NULL;
	 str=realloc(str, 1);
	 str[0] = '\0';
}
void append(string str1){
	string p  = realloc(str, strlen(str)+strlen(str1)+1);
	 if(p==NULL) {
		fprintf(stderr,"\nString Size out of memory!\n");
		exit(1);
	 }
	 str = p;
	 strcat(str,str1);
}
string Tostring(){
	return str;
}
string ChangeData(string str1){
	string p = checked_malloc(2);
	int val = atoi(&str1[1]);
	if(val>127){
		EM_error(EM_tokPos,"illegal num:%d.",val);
		val =0;
	}
	
	p[0] = val;
	p[1] = '\0';
	return p;
}

void EM_newline_Check(string str){
	int size = strlen(str);
	if(size<3)return;
	for(int i =1;i<(size-1);i++){
		if(str[i]=='\n'){
			EM_newline();
		}
	}
}
%}
%state COMMENT STRING1
%%
<COMMENT>{
	"/*"  {adjust(); comment_count++;}
	"*/"	{adjust();if(comment_count==0){BEGIN(INITIAL);}else comment_count--;}
	\n|(\r\n)	 {adjust(); EM_newline(); continue;}
	.	{adjust();continue;}
	 <<EOF>> {adjust(); EM_error(EM_tokPos,"EOF in comment"); return 0;}
}
<STRING1>{
	\"			{adjust();yylval.sval=str;BEGIN(INITIAL);return STRING;}
	\n|(\r\n) 	{adjust(); EM_newline();  EM_error(EM_tokPos,"need \" or \\ ...\\.");}
	\\[0-9]{3}	{adjust();append(ChangeData(yytext));continue;}
	[a-zA-Z0-9]+ {adjust();append(yytext);continue;}
	\\t			{adjust();append("\t");continue;}
	\\\"		{adjust();append("\"");continue;}
	\\n          {adjust(); append("\n");continue;}
	\\\\		{adjust();  append("\\");continue;}
	\\(\n|\r|\r\n|\t|" ")+\\   {adjust(); EM_newline_Check(yytext); continue;}
	.	 {adjust(); EM_error(EM_tokPos,"illegal string");}
	<<EOF>> {adjust(); EM_error(EM_tokPos,"EOF in String"); return 0;}
}
<INITIAL>{
	\"   {adjust();init();BEGIN(STRING1);}
	"/*"  {adjust(); BEGIN(COMMENT);comment_count=0;}	
	[ \t]*   {adjust(); continue;}
	\n   {adjust(); EM_newline(); continue;}
	\r\n {adjust(); EM_newline(); continue;}
	","  {adjust(); return COMMA;}
	":"  {adjust(); return COLON;}
	";"  {adjust(); return SEMICOLON;}
	"("  {adjust(); return LPAREN;}
	")"  {adjust(); return RPAREN;}
	"["  {adjust(); return LBRACK;}
	"]"  {adjust(); return RBRACK;}
	"{"  {adjust(); return LBRACE;}
	"}"  {adjust(); return RBRACE;}
	"."  {adjust(); return DOT;}
	"+"  {adjust(); return PLUS;}
	"-"  {adjust(); return MINUS;}
	"*"  {adjust(); return TIMES;}
	"/"  {adjust(); return DIVIDE;}
	"="  {adjust(); return EQ;}
	"<>" {adjust(); return NEQ;}
	"<"  {adjust(); return LT;}
	"<=" {adjust(); return LE;}
	">"  {adjust(); return GT;}
	">=" {adjust(); return GE;}
	"&"  {adjust(); return AND;}
	"|"  {adjust(); return OR;}
	":=" {adjust(); return ASSIGN;}
	array   {adjust(); return ARRAY;}
	if      {adjust(); return IF;}
	then    {adjust(); return THEN;}
	else    {adjust(); return ELSE;}
	while   {adjust(); return WHILE;}
	for     {adjust(); return FOR;}
	to      {adjust(); return TO;}
	do      {adjust(); return DO;}
	let     {adjust(); return LET;}
	in      {adjust(); return IN;}
	end     {adjust(); return END;}
	of      {adjust(); return OF;}
	break   {adjust(); return BREAK;}
	nil     {adjust(); return NIL;}
	function {adjust(); return FUNCTION;}
	var     {adjust(); return VAR;}
	type    {adjust(); return TYPE;}
	[a-zA-Z]+[a-zA-Z0-9_]*  {adjust(); yylval.sval = String(yytext); return ID;}
	[0-9]+   {adjust(); yylval.ival=atoi(yytext); return INT;}
	.    {adjust(); EM_error(EM_tokPos,"illegal token");}
	<<EOF>> {adjust(); EM_error(EM_tokPos,"EOF in text"); return 0;}
}
