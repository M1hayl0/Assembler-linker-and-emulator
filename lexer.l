%{
  #include <stdlib.h>
  #include <stdio.h>
  #include "parser.h"
  #include "helpers.h"
%}

%option outfile="lexer.c" header-file="lexer.h"

%option noyywrap

%%
".global" { return GLOBAL; }
".extern" { return EXTERN; }
".section" { return SECTION; }
".word" { return WORD; }
".skip" { return SKIP; }
".ascii" { return ASCII; }
".equ" { return EQU; }
".end" { return END; }
"halt" { return HALT; }
"int" { return INT; }
"iret" { return IRET; }
"call" { return CALL; }
"ret" { return RET; }
"jmp" { return JMP; }
"beq" { return BEQ; }
"bne" { return BNE; }
"bgt" { return BGT; }
"push" { return PUSH; }
"pop" { return POP; }
"xchg" { return XCHG; }
"add" { return ADD; }
"sub" { return SUB; }
"mul" { return MUL; }
"div" { return DIV; }
"not" { return NOT; }
"and" { return AND; }
"or" { return OR; }
"xor" { return XOR; }
"shl" { return SHL; }
"shr" { return SHR; }
"ld" { return LD; }
"st" { return ST; }
"csrrd" { return CSRRD; }
"csrwr" { return CSRWR; }
r(0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15)|sp|pc { yylval.strReg = copyStr(yytext); return REG; }
0x[0-9a-fA-F]+ { yylval.numLit = strtoul(yytext, NULL, 16); return LIT; }
[0-9]+ { yylval.numLit = atoi(yytext); return LIT; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.strSym = copyStr(yytext); return SYM; }
"$" { return VALUE; }
"%" { return REG_START; }
"[" { return MEM_START; }
"]" { return MEM_END; }
"+" { return PLUS; }
"," { return COMMA; }
":" { return COLON; }
\"(.*)\" { yylval.strAscii = copyStr(yytext); return STRING; }
\n { return EOL; }
[ \t] { /* ignore whitespace */ }
#(.*) { /* ignore comments */ }
. { printf("Wrong text %c\n", *yytext); }
%%

void yyerror(const char *s) {
  fprintf(stderr, "error: %s\n", s);
}
