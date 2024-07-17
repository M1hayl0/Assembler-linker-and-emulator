#include "helpers.h"
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
  if (strcmp(regName, "r0") == 0) return 0;
  else if (strcmp(regName, "r1") == 0) return 1;
  else if (strcmp(regName, "r2") == 0) return 2;
  else if (strcmp(regName, "r3") == 0) return 3;
  else if (strcmp(regName, "r4") == 0) return 4;
  else if (strcmp(regName, "r5") == 0) return 5;
  else if (strcmp(regName, "r6") == 0) return 6;
  else if (strcmp(regName, "r7") == 0) return 7;
  else if (strcmp(regName, "r8") == 0) return 8;
  else if (strcmp(regName, "r9") == 0) return 9;
  else if (strcmp(regName, "r10") == 0) return 10;
  else if (strcmp(regName, "r11") == 0) return 11;
  else if (strcmp(regName, "r12") == 0) return 12;
  else if (strcmp(regName, "r13") == 0) return 13;
  else if (strcmp(regName, "r14") == 0 || strcmp(regName, "sp") == 0) return 14;
  else if (strcmp(regName, "r15") == 0 || strcmp(regName, "pc") == 0) return 15;
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