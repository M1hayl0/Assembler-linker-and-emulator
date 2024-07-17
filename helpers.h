#ifndef HELPERS_H
#define HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

enum operandType {
  valueLitType,
  valueSymType,
  litType,
  symType,
  regType,
  regMemType,
  regMemLitType,
  regMemSymType,
  litJumpType,
  symJumpType
};

struct line {
  struct directive *directive;
  struct instruction *instruction;
  struct label *label;
  struct line *next;
};

struct directive {
  char *name;
  struct operandArgs *operands;
  char *string;
};

struct instruction {
  char *name;
  struct operandArgs *operand1;
  struct operandArgs *operand2;
  struct operandArgs *operand3;
};

struct label {
  struct operandArgs *operand;
};

struct operandArgs {
  enum operandType type;
  int regNum;
  char *symbol;
  int literal;
  struct operandArgs *next;
};

extern struct line *head;
extern struct line *tail;

char* copyStr(const char*);
char* removeFirstAndLastChar(const char*);
int getRegNum(char *);

struct line *makeLine(struct directive *, struct instruction *, struct label *);
struct directive *makeDirective(const char *, struct operandArgs *, char *);
struct instruction *makeInstruction(const char *, struct operandArgs *, struct operandArgs *, struct operandArgs *);
struct label *makeLabel(struct operandArgs *);
struct operandArgs *makeOperand(enum operandType, int, char *, int);

void freeOperandArgs(struct operandArgs *);
void freeDirective(struct directive *);
void freeInstruction(struct instruction *);
void freeLabel(struct label *);
void freeLines(struct line *);

#ifdef __cplusplus
}
#endif

#endif // HELPERS_H