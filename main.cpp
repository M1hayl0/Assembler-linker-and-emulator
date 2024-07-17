#include "helpers.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>

void printOperand(struct operandArgs *op) {
    if(op->type == valueLitType) printf("$%d", op->literal);
    else if(op->type == valueSymType) printf("$%s", op->symbol);
    else if(op->type == litType) printf("%d", op->literal);
    else if(op->type == symType) printf("%s", op->symbol);
    else if(op->type == regType) printf("%%r%d", op->regNum);
    else if(op->type == regMemType) printf("[%%r%d]", op->regNum);
    else if(op->type == regMemLitType) printf("[%%r%d + %d]", op->regNum, op->literal);
    else if(op->type == regMemSymType) printf("[%%r%d + %s]", op->regNum, op->symbol);
    else if(op->type == litJumpType) printf("%d", op->literal);
    else if(op->type == symJumpType) printf("%s", op->symbol);
}

int main(int argc, char* argv[]) {
    if (yyparse()) return 1;

    for(struct line *currentLine = head; currentLine; currentLine = currentLine->next) {
        if(currentLine->directive) {
            if (strcmp(currentLine->directive->name, ".global") == 0 ||
                strcmp(currentLine->directive->name, ".extern") == 0 ||
                strcmp(currentLine->directive->name, ".section") == 0 ||
                strcmp(currentLine->directive->name, ".word") == 0 ||
                strcmp(currentLine->directive->name, ".skip") == 0 ||
                strcmp(currentLine->directive->name, ".equ") == 0 ||
                strcmp(currentLine->directive->name, ".end") == 0
                ) {
                printf("%s ", currentLine->directive->name);
                for(struct operandArgs *curOp = currentLine->directive->operands; curOp; curOp = curOp->next) {
                    printOperand(curOp);
                    if(curOp->next) printf(", ");
                }
                printf("\n");
            } else if (strcmp(currentLine->directive->name, ".ascii") == 0) {
                printf("%s \"%s\"\n", currentLine->directive->name, currentLine->directive->string);
            }
        } else if(currentLine->instruction) {
            printf("%s ", currentLine->instruction->name);
            if(currentLine->instruction->operand1) printOperand(currentLine->instruction->operand1); 
            if(currentLine->instruction->operand2) { printf(", "); printOperand(currentLine->instruction->operand2); }
            if(currentLine->instruction->operand3) { printf(", "); printOperand(currentLine->instruction->operand3); }
            printf("\n");
        } else if(currentLine->label) {
            printOperand(currentLine->label->operand);
            printf(":\n");
        }
    }

    freeLines(head);
    return 0;
}