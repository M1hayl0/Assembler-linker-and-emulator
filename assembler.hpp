#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

using namespace std;

class Assembler {
private:
  enum symbolType { NOTYP, SCTN };
  enum symbolBind { LOC, GLOB };

  struct forwardRefsList {
    int patch;
    forwardRefsList *next;
  };

  struct symbolTableRow {
    int num;
    int value;
    symbolType type;
    symbolBind bind;
    int sectionIndex;
    string name;

    bool defined;
    forwardRefsList *head;
  };
  
  enum relaType { 
    R_X86_64_32, //symbol value + addend
    R_X86_64_PC32  //symbol value + addend - offset
  };

  struct relaTableRow {
    int offset;
    relaType type;
    int symbol;
    int addend;
  };

  struct sectionStruct {
    int sectionNum;
    string sectionName;
    vector<uint8_t> sectionData8bitValues;
    int locationCounter;
    vector<relaTableRow> relaTable;
  };

  vector<symbolTableRow> symbolTable;
  vector<sectionStruct> sections;
  struct line *code;

public:
  Assembler(struct line *);
  void assemble();

  void globalAssemble(struct directive *);
  void externAssemble(struct directive *);
  void sectionAssemble(struct directive *);
  void wordAssemble(struct directive *);
  void skipAssemble(struct directive *);
  void asciiAssemble(struct directive *);
  void equAssemble(struct directive *);

  void haltAssemble(struct instruction *);
  void intAssemble(struct instruction *);
  void iretAssemble(struct instruction *);
  void callAssemble(struct instruction *);
  void retAssemble(struct instruction *);
  void jmpAssemble(struct instruction *);
  void beqBneBgtAssemble(struct instruction *, int);
  void pushAssemble(struct instruction *);
  void popAssemble(struct instruction *);
  void xchgAssemble(struct instruction *);
  void notAssemble(struct instruction *);
  void aritLogShiftAssemble(struct instruction *, int, int);
  void ldAssemble(struct instruction *);
  void stAssemble(struct instruction *);
  void csrrdAssemble(struct instruction *);
  void csrwrAssemble(struct instruction *);

  void labelAssemble(struct label *);

  void push32BitValue(int, sectionStruct &);
  int makeInstructionCode(int, int, int, int, int, int, int, int);

  void printSymbolTable();
  void printRelaTables(const sectionStruct&);
  void printSections();
};

#endif // ASSEMBLER_H