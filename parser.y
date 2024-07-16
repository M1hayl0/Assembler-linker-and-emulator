%{
#include <stdio.h>
#include <stdlib.h>

void yyerror(char *s);
int yylex();
%}

%token GLOBAL EXTERN SECTION WORD SKIP ASCII EQU END 
%token HALT INT IRET CALL RET JMP BEQ BNE BGT PUSH POP XCHG ADD SUB MUL DIV NOT AND OR XOR SHL SHR LD ST CSRRD CSRWR
%token LIT SYM REG VALUE REG_START MEM_START MEM_END PLUS COMMA COLON STRING EOL

%%

program:
| program line
;

line:
directive eol
| instruction eol
| label eol
| directive
| instruction
| label
| eol
;

directive: 
global symbols
| extern symbols
| section sym
| word symlits 
| skip lit
| ascii string
| equ sym comma lit
| end
;

symbols:
sym
| symbols comma sym
;

symlits:
sym
| lit
| symlits comma sym
| symlits comma lit
;

instruction:
halt
| int
| iret
| call operandJump
| ret
| jmp operandJump
| beq reg comma reg comma operandJump
| bne reg comma reg comma operandJump
| bgt reg comma reg comma operandJump
| push reg
| pop reg
| xchg reg comma reg
| add reg comma reg
| sub reg comma reg
| mul reg comma reg
| div reg comma reg
| not reg
| and reg comma reg
| or reg comma reg
| xor reg comma reg
| shl reg comma reg
| shr reg comma reg
| ld operand comma reg
| st reg comma operand
| csrrd reg comma reg
| csrwr reg comma reg
;

global: GLOBAL { printf("GLOBAL "); }
extern: EXTERN { printf("EXTERN "); }
section: SECTION { printf("SECTION " ); }
word: WORD { printf("WORD "); }
skip: SKIP { printf("SKIP "); }
ascii: ASCII { printf("ASCII "); }
equ: EQU { printf("EQU "); }
end: END { printf("END"); }

halt: HALT { printf("HALT"); }
int: INT { printf("INT"); }
iret: IRET { printf("IRET"); }
call: CALL { printf("CALL "); }
ret: RET { printf("RET"); }
jmp: JMP { printf("JMP "); }
beq: BEQ { printf("BEQ "); }
bne: BNE { printf("BNE "); }
bgt: BGT { printf("BGT "); }
push: PUSH { printf("PUSH "); }
pop: POP { printf("POP "); }
xchg: XCHG { printf("XCHG "); }
add: ADD { printf("ADD "); }
sub: SUB { printf("SUB "); }
mul: MUL { printf("MUL "); }
div: DIV { printf("DIV "); }
not: NOT { printf("NOT "); }
and: AND { printf("AND "); }
or: OR { printf("OR "); }
xor: XOR { printf("XOR "); }
shl: SHL { printf("SHL "); }
shr: SHR { printf("SHR "); }
ld: LD { printf("LD "); }
st: ST { printf("ST "); }
csrrd: CSRRD { printf("CSRRD "); }
csrwr: CSRWR { printf("CSRWR "); }

operand:
VALUE LIT { printf("$LITERAL"); }
| VALUE SYM { printf("$SYMBOL"); }
| LIT { printf("LITERAL"); }
| SYM { printf("SYMBOL"); }
| REG_START REG { printf("%%REG"); }
| MEM_START REG_START REG MEM_END { printf("[%%REG]"); }
| MEM_START REG_START REG PLUS LIT MEM_END { printf("[%%REG+LITERAL]"); }
| MEM_START REG_START REG PLUS SYM MEM_END { printf("[%%REG+SYMBOL]"); }
;

operandJump:
LIT { printf("LITERAL"); }
| SYM { printf("SYMBOL"); }

reg: REG_START REG { printf("%%REG"); }
sym: SYM { printf("SYMBOL"); }
lit: LIT { printf("LITERAL"); }
comma: COMMA { printf(", "); }
string: STRING { printf("STRING"); }
eol: EOL { printf("\n"); }
label: SYM COLON { printf("LABEL:"); }

%%

int main() {
  printf("> "); 
  yyparse();
}

void yyerror(char *s) {
  fprintf(stderr, "error: %s\n", s);
}