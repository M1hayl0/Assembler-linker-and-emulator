%{
  #include <stdio.h>
  #include "argumentTrasfer.h"

  void yyerror(const char *s);
  int yylex(void);
%}

%output "parser.c"
%defines "parser.h"

%union {
  int numLit;
  char *strReg;
  char *strSym;
  char *strAscii;
  
  struct directive *directive;
  struct instruction *instruction;
  struct label *label;
  struct operandArgs *operandArgs;
  char *string;
}

%token GLOBAL EXTERN SECTION WORD SKIP ASCII EQU END 
%token HALT INT IRET CALL RET JMP BEQ BNE BGT PUSH POP XCHG ADD SUB MUL DIV NOT AND OR XOR SHL SHR LD ST CSRRD CSRWR
%token VALUE REG_START MEM_START MEM_END PLUS MINUS COMMA COLON EOL

%token <numLit> LIT
%token <strReg> REGGPR
%token <strReg> REGCSR
%token <strSym> SYM
%token <strAscii> STRING

%type <directive> directive;
%type <instruction> instruction;
%type <label> label;

%type <operandArgs> operand;
%type <operandArgs> operandJump;
%type <operandArgs> regGpr;
%type <operandArgs> regCsr;
%type <operandArgs> sym;
%type <operandArgs> lit;
%type <operandArgs> symbols;
%type <operandArgs> symlits;
%type <operandArgs> expression;
%type <string> string;

%%

program:
| program line
;

line:
directive eol { makeLine($1, NULL, NULL); }
| instruction eol { makeLine(NULL, $1, NULL); }
| label eol { makeLine(NULL, NULL, $1); }
| directive { makeLine($1, NULL, NULL); }
| instruction { makeLine(NULL, $1, NULL); }
| label { makeLine(NULL, NULL, $1); }
| eol
;


directive: 
global symbols { $$ = makeDirective(".global", $2, NULL); }
| extern symbols { $$ = makeDirective(".extern", $2, NULL); }
| section sym { $$ = makeDirective(".section", $2, NULL); }
| word symlits { $$ = makeDirective(".word", $2, NULL); }
| skip lit { $$ = makeDirective(".skip", $2, NULL); }
| ascii string { $$ = makeDirective(".ascii", NULL, $2); }
| equ sym comma expression { $2->next = $4; $$ = makeDirective(".equ", $2, NULL); }
| end { $$ = makeDirective(".end", NULL, NULL); }
;

global: GLOBAL
extern: EXTERN
section: SECTION
word: WORD
skip: SKIP
ascii: ASCII
equ: EQU
end: END

symbols:
sym { $$ = $1; }
| sym comma symbols { $1->next = $3; $$ = $1; }
;

symlits:
sym { $$ = $1; }
| lit { $$ = $1; }
| sym comma symlits { $1->next = $3; $$ = $1; }
| lit comma symlits { $1->next = $3; $$ = $1; }
;

expression:
sym { $$ = $1; }
| lit { $$ = $1; }
| sym PLUS expression { $1->next = $3; $$ = $1; }
| lit PLUS expression { $1->next = $3; $$ = $1; }
| sym MINUS expression { $3->minus = 1; $1->next = $3; $$ = $1; }
| lit MINUS expression { $3->minus = 1; $1->next = $3; $$ = $1; }
| PLUS expression { $$ = $2; }
| MINUS expression { $2->minus = 1; $$ = $2;}

string: STRING[strAscii] { $$ = removeFirstAndLastChar($strAscii); }


instruction:
halt { $$ = makeInstruction("halt", NULL, NULL, NULL); }
| int { $$ = makeInstruction("int", NULL, NULL, NULL); }
| iret { $$ = makeInstruction("iret", NULL, NULL, NULL); }
| call operandJump { $$ = makeInstruction("call", $2, NULL, NULL); }
| ret { $$ = makeInstruction("ret", NULL, NULL, NULL); }
| jmp operandJump { $$ = makeInstruction("jmp", $2, NULL, NULL); }
| beq regGpr comma regGpr comma operandJump { $$ = makeInstruction("beq", $2, $4, $6); }
| bne regGpr comma regGpr comma operandJump { $$ = makeInstruction("bne", $2, $4, $6); }
| bgt regGpr comma regGpr comma operandJump { $$ = makeInstruction("bgt", $2, $4, $6); }
| push regGpr { $$ = makeInstruction("push", $2, NULL, NULL); }
| pop regGpr { $$ = makeInstruction("pop", $2, NULL, NULL); }
| xchg regGpr comma regGpr { $$ = makeInstruction("xchg", $2, $4, NULL); }
| add regGpr comma regGpr { $$ = makeInstruction("add", $2, $4, NULL); }
| sub regGpr comma regGpr { $$ = makeInstruction("sub", $2, $4, NULL); }
| mul regGpr comma regGpr { $$ = makeInstruction("mul", $2, $4, NULL); }
| div regGpr comma regGpr { $$ = makeInstruction("div", $2, $4, NULL); }
| not regGpr { $$ = makeInstruction("not", $2, NULL, NULL); }
| and regGpr comma regGpr { $$ = makeInstruction("and", $2, $4, NULL); }
| or regGpr comma regGpr { $$ = makeInstruction("or", $2, $4, NULL); }
| xor regGpr comma regGpr { $$ = makeInstruction("xor", $2, $4, NULL); }
| shl regGpr comma regGpr { $$ = makeInstruction("shl", $2, $4, NULL); }
| shr regGpr comma regGpr { $$ = makeInstruction("shr", $2, $4, NULL); }
| ld operand comma regGpr { $$ = makeInstruction("ld", $2, $4, NULL); }
| st regGpr comma operand { $$ = makeInstruction("st", $2, $4, NULL); }
| csrrd regCsr comma regGpr { $$ = makeInstruction("csrrd", $2, $4, NULL); }
| csrwr regGpr comma regCsr { $$ = makeInstruction("csrwr", $2, $4, NULL); }
;

halt: HALT
int: INT
iret: IRET
call: CALL
ret: RET
jmp: JMP
beq: BEQ
bne: BNE
bgt: BGT
push: PUSH
pop: POP
xchg: XCHG
add: ADD
sub: SUB
mul: MUL
div: DIV
not: NOT
and: AND
or: OR
xor: XOR
shl: SHL
shr: SHR
ld: LD
st: ST
csrrd: CSRRD
csrwr: CSRWR

operand:
VALUE LIT[numLit] { $$ = makeOperand(valueLitType, -1, NULL, $numLit); }
| VALUE SYM[strSym] { $$ = makeOperand(valueSymType, -1, $strSym, -1); }
| LIT[numLit] { $$ = makeOperand(litType, -1, NULL, $numLit); }
| SYM[strSym] { $$ = makeOperand(symType, -1, $strSym, -1); }
| REG_START REGGPR[strReg] { $$ = makeOperand(regType, getRegNum($strReg), NULL, -1); }
| MEM_START REG_START REGGPR[strReg] MEM_END { $$ = makeOperand(regMemType, getRegNum($strReg), NULL, -1); }
| MEM_START REG_START REGGPR[strReg] PLUS LIT[numLit] MEM_END { $$ = makeOperand(regMemLitType, getRegNum($strReg), NULL, $numLit); }
| MEM_START REG_START REGGPR[strReg] PLUS SYM[strSym] MEM_END { $$ = makeOperand(regMemSymType, getRegNum($strReg), $strSym, -1); }
;

operandJump:
LIT[numLit] { $$ = makeOperand(litJumpType, -1, NULL, $numLit); }
| SYM[strSym] { $$ = makeOperand(symJumpType, -1, $strSym, -1); }

regGpr: REG_START REGGPR[strReg] { $$ = makeOperand(regType, getRegNum($strReg), NULL, -1); }
regCsr: REG_START REGCSR[strReg] { $$ = makeOperand(regType, getRegNum($strReg), NULL, -1); }
lit: LIT[numLit] { $$ = makeOperand(litType, -1, NULL, $numLit); }
sym: SYM[strSym] { $$ = makeOperand(symType, -1, $strSym, -1); }


label: SYM[strSym] COLON { $$ = makeLabel(makeOperand(symType, -1, $strSym, -1)); }


comma: COMMA
eol: EOL

%%