#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <vector>

using namespace std;

class Assembler {
private:
  struct line *code;
  string outputFileName;
  

  enum symbolType { NOTYP, SCTN };
  enum symbolBind { LOC, GLOB };
  
  enum relaType {
    MY_R_X86_64_32S //symbol value + addend
  };

  struct forwardRefsList {
    int offset;
    int sectionToPatchNum;
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
    int symtabIndex;
  };

  vector<symbolTableRow> symbolTable;
  vector<sectionStruct> sections;


  struct equTableRow {
    struct operandArgs *symbol;
    struct operandArgs *expresion;
    int sectionIndex = -1;
    bool done = false;
  };

  vector<equTableRow> equTable;

  struct ldStRegMemSymRow {
    struct instruction *instruction;
    bool ld;
    int sectionNum;
    int offset;
  };

  vector<ldStRegMemSymRow> ldStRegMemSym;

public:
  Assembler(struct line *, char *);
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

  void externSymbolsBackpatch();
  void equBackpatch();
  void ldStBackpatch();

  void push32BitValue(int, sectionStruct &);
  int makeInstructionCode(int, int, int, int, int, int, int, int);

  void printSymbolTable();
  void printRelaTables(const sectionStruct&);
  void printSections();

  void elfWrite();
};

#endif // ASSEMBLER_H