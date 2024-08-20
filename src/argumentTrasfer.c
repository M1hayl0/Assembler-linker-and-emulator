#include "./../inc/argumentTrasfer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct line *head = NULL;
struct line *tail = NULL;

char *copyStr(const char *str) {
  if(str == NULL) return NULL;
  int len = strnlen(str, 100);
  char *buf = (char *) malloc(len + 1);
  strncpy(buf, str, len);
  buf[len] = '\0';
  return buf;
}

char* removeFirstAndLastChar(const char* str) {
  if(str == NULL) return NULL;
  int len = strlen(str);
  char* newStr = (char*) malloc(len - 1);
  strncpy(newStr, str + 1, len - 2);
  newStr[len - 2] = '\0';
  return newStr;
}

int getRegNum(char *regName) {
  if(strcmp(regName, "r0") == 0) return 0;
  else if(strcmp(regName, "r1") == 0) return 1;
  else if(strcmp(regName, "r2") == 0) return 2;
  else if(strcmp(regName, "r3") == 0) return 3;
  else if(strcmp(regName, "r4") == 0) return 4;
  else if(strcmp(regName, "r5") == 0) return 5;
  else if(strcmp(regName, "r6") == 0) return 6;
  else if(strcmp(regName, "r7") == 0) return 7;
  else if(strcmp(regName, "r8") == 0) return 8;
  else if(strcmp(regName, "r9") == 0) return 9;
  else if(strcmp(regName, "r10") == 0) return 10;
  else if(strcmp(regName, "r11") == 0) return 11;
  else if(strcmp(regName, "r12") == 0) return 12;
  else if(strcmp(regName, "r13") == 0) return 13;
  else if(strcmp(regName, "r14") == 0 || strcmp(regName, "sp") == 0) return 14;
  else if(strcmp(regName, "r15") == 0 || strcmp(regName, "pc") == 0) return 15;
  else if(strcmp(regName, "status") == 0) return 16;
  else if(strcmp(regName, "handler") == 0) return 17;
  else if(strcmp(regName, "cause") == 0) return 18;
  return -1;
}

struct line *makeLine(struct directive *directive, struct instruction *instruction, struct label *label) {
  struct line *l = (struct line *) malloc(sizeof(struct line));
  l->directive = directive;
  l->instruction = instruction;
  l->label = label;
  l->next = NULL;
  if(!head) head = tail = l;
  else tail = tail->next = l;
  return l;
}

struct directive *makeDirective(const char *name, struct operandArgs *operands, char *string) {
  struct directive *dir = (struct directive *) malloc(sizeof(struct directive));
  dir->name = copyStr(name);
  dir->operands = operands;
  dir->string = copyStr(string);
  return dir;
}

struct instruction *makeInstruction(const char *name, struct operandArgs *operand1, struct operandArgs *operand2, struct operandArgs *operand3) {
  struct instruction *in = (struct instruction *) malloc(sizeof(struct instruction));
  in->name = copyStr(name);
  in->operand1 = operand1;
  in->operand2 = operand2;
  in->operand3 = operand3;
  return in;
}

struct label *makeLabel(struct operandArgs *operand) {
  struct label *lab = (struct label *) malloc(sizeof(struct label));
  lab->operand = operand;
  return lab;
}

struct operandArgs *makeOperand(enum operandType type, int regNum, char *symbol, int literal) {
  struct operandArgs* op = (struct operandArgs*) malloc(sizeof(struct operandArgs));
  op->type = type;
  op->regNum = regNum;
  op->symbol = copyStr(symbol);
  op->literal = literal;
  op->minus = 0;
  op->next = NULL;
  return op;
}


void freeOperandArgs(struct operandArgs *op) {
  struct operandArgs *current = op;
  struct operandArgs *next;
  
  while(current != NULL) {
    next = current->next;
    if(current->symbol) free(current->symbol);
    free(current);
    current = next;
  }
}

void freeDirective(struct directive *dir) {
  if(dir) {
    if(dir->name) free(dir->name);
    if(dir->string) free(dir->string);
    freeOperandArgs(dir->operands);
    free(dir);
  }
}

void freeInstruction(struct instruction *instr) {
  if(instr) {
    if(instr->name) free(instr->name);
    freeOperandArgs(instr->operand1);
    freeOperandArgs(instr->operand2);
    freeOperandArgs(instr->operand3);
    free(instr);
  }
}

void freeLabel(struct label *lbl) {
  if(lbl) {
    freeOperandArgs(lbl->operand);
    free(lbl);
  }
}

void freeLines(struct line *linesHead) {
  struct line *current = linesHead;
  struct line *next;
    
  while(current != NULL) {
    next = current->next;
    freeDirective(current->directive);
    freeInstruction(current->instruction);
    freeLabel(current->label);
    free(current);
    current = next;
  }
}


void printOperand(struct operandArgs *op) {
  if(op->type == valueLitType) printf("$%d", op->literal);
  else if(op->type == valueSymType) printf("$%s", op->symbol);
  else if(op->type == litType) printf("%d", op->literal);
  else if(op->type == symType) printf("%s", op->symbol);
  else if(op->type == regType && op->regNum < 16) printf("%%r%d", op->regNum);
  else if(op->type == regType && op->regNum == 16) printf("%%status");
  else if(op->type == regType && op->regNum == 17) printf("%%handler");
  else if(op->type == regType && op->regNum == 18) printf("%%cause");
  else if(op->type == regMemType) printf("[%%r%d]", op->regNum);
  else if(op->type == regMemLitType) printf("[%%r%d + %d]", op->regNum, op->literal);
  else if(op->type == regMemSymType) printf("[%%r%d + %s]", op->regNum, op->symbol);
  else if(op->type == litJumpType) printf("%d", op->literal);
  else if(op->type == symJumpType) printf("%s", op->symbol);
}

void printLines(struct line *head) {
  for(struct line *currentLine = head; currentLine; currentLine = currentLine->next) {
    if(currentLine->directive) {
      if(strcmp(currentLine->directive->name, ".global") == 0 ||
        strcmp(currentLine->directive->name, ".extern") == 0 ||
        strcmp(currentLine->directive->name, ".section") == 0 ||
        strcmp(currentLine->directive->name, ".word") == 0 ||
        strcmp(currentLine->directive->name, ".skip") == 0 ||
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
      } else if (strcmp(currentLine->directive->name, ".equ") == 0) {
        printf("%s ", currentLine->directive->name);
        printOperand(currentLine->directive->operands);
        printf(", ");
        for(struct operandArgs *curOp = currentLine->directive->operands->next; curOp; curOp = curOp->next) {
          if(curOp->minus == 0 && curOp != currentLine->directive->operands->next) printf(" + ");
          else if(curOp->minus == 1 && curOp != currentLine->directive->operands->next) printf(" - ");
          else if(curOp->minus == 1) printf("-");
          printOperand(curOp);
        }
        printf("\n");
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
}