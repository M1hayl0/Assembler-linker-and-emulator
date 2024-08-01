#ifndef LINKER_H
#define LINKER_H

#include <map>
#include <string>
#include <vector>

using namespace std;

class Linker {
private:
  string outputFile;
  map<string, uint> place;
  bool hexBool;
  bool relocatableBool;
  vector<string> inputFiles;


  enum symbolType { NOTYP, SCTN };
  enum symbolBind { LOC, GLOB };

  struct symbolTableRow {
    int num;
    int value;
    symbolType type;
    symbolBind bind;
    int sectionIndex;
    string name;

    int nameElf = 0;

    string sectionName;
  };
  
  enum relaType { 
    MY_R_X86_64_32S, //symbol value + addend
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

    int secIndexElf;

    int addressInLinkedSection;
  };

  struct inputFileStruct {
    string inputFileName;
    vector<symbolTableRow> symbolTable;
    vector<sectionStruct> sections;
  };

  vector<inputFileStruct> inputFilesSections;


  struct mappingStruct {
    string sectionName;
    uint startAddress;
    uint size;
  };

  vector<mappingStruct> mappedLinkedSections;
  uint notFixedSectionsStartAddress = 0;
  uint notFixedSectionsTotalSize = 0;


  vector<symbolTableRow> newSymbolTable;

  vector<symbolTableRow> relocatableNewSymbolTable;
  vector<sectionStruct> relocatableNewSections;
  
public:
  Linker(string, map<string, uint>, bool, bool, vector<string>);
  void link();

  void elfRead(string);
  void mapping();
  void symbolDetermination();
  void symbolResolution();
  void makeRelocatableNewSections();
  void hexWrite();
  void elfWrite();

  void printSymbolTable(const vector<symbolTableRow>&);
  void printRelaTables(const sectionStruct&);
  void printSections(const vector<sectionStruct>&);
  void printInputFilesSections();
};

#endif // LINKER_H