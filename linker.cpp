#include "linker.hpp"

#include <fstream>
#include <iostream>
#include <cstring>
#include <elf.h>
#include <map>
#include <vector>
#include <string.h>


Linker::Linker(string outputFile, map<string, int> place, bool hex, bool relocatable, vector<string> inputFiles) {
  this->outputFile = outputFile;
  this->place = place;
  this->hex = hex;
  this->relocatable = relocatable;
  this->inputFiles = inputFiles;
}

void Linker::link() {
  for(auto file : inputFiles) elfRead(file);
  printInputFilesSections();

  mapping();
  symbolDetermination();
  symbolResolution();
}

void Linker::elfRead(string inputFileName) {
  ifstream ifs(inputFileName, ios::binary);
  if(!ifs) {
    cerr << "Failed to open file for reading." << endl;
    return;
  }
    
  inputFileStruct inputFile{inputFileName, vector<symbolTableRow>(), vector<sectionStruct>()};

  // ELF Header
  Elf64_Ehdr ehdr;
  ifs.read(reinterpret_cast<char*>(&ehdr), sizeof(ehdr));
  if(!ifs) {
    cerr << "Failed to read ELF header." << endl;
    return;
  }

  // Check ELF magic number
  if(memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
    cerr << "Not a valid ELF file." << endl;
    return;
  }

  // Section Headers
  vector<Elf64_Shdr> shdrs(ehdr.e_shnum);
  ifs.seekg(ehdr.e_shoff, ios::beg);
  ifs.read(reinterpret_cast<char*>(shdrs.data()), ehdr.e_shnum * sizeof(Elf64_Shdr));
  if(!ifs) {
    cerr << "Failed to read section headers." << endl;
    return;
  }

  // Read section header string table
  string shstrtab;
  ifs.seekg(shdrs[ehdr.e_shstrndx].sh_offset, ios::beg);
  shstrtab.resize(shdrs[ehdr.e_shstrndx].sh_size);
  ifs.read(&shstrtab[0], shstrtab.size());
  if(!ifs) {
    cerr << "Failed to read section header string table." << endl;
    return;
  }

  inputFile.sections.clear();
  inputFile.symbolTable.clear();

  // Read sections and their data
  for(size_t i = 0; i < shdrs.size(); i++) {
    const Elf64_Shdr &shdr = shdrs[i];
    string sectionName = shstrtab.c_str() + shdr.sh_name;

    if(shdr.sh_type == SHT_PROGBITS) {
      sectionStruct section;
      section.sectionNum = 0;
      section.sectionName = sectionName;
      section.secIndexElf = i;

      ifs.seekg(shdr.sh_offset, ios::beg);
      section.sectionData8bitValues.resize(shdr.sh_size);
      ifs.read(reinterpret_cast<char*>(section.sectionData8bitValues.data()), shdr.sh_size);
      if(!ifs) {
        cerr << "Failed to read section data." << endl;
        return;
      }

      section.locationCounter = section.sectionData8bitValues.size();

      inputFile.sections.push_back(section);
    } else if(shdr.sh_type == SHT_SYMTAB) {
      ifs.seekg(shdr.sh_offset, ios::beg);
      size_t numSymbols = shdr.sh_size / sizeof(Elf64_Sym);
      for(size_t j = 0; j < numSymbols; j++) {
        Elf64_Sym elfSym;
        ifs.read(reinterpret_cast<char*>(&elfSym), sizeof(elfSym));
        if(!ifs) {
          cerr << "Failed to read symbol table entry." << endl;
          return;
        }

        symbolTableRow symRow;
        symRow.num = j;
        symRow.value = elfSym.st_value;
        symRow.type = (elfSym.st_info & 0xf) == STT_SECTION ? SCTN : NOTYP;
        symRow.bind = (elfSym.st_info >> 4) == STB_GLOBAL ? GLOB : LOC;
        symRow.sectionIndex = elfSym.st_shndx;
        symRow.name = ""; // Name will be filled later from strtab
        symRow.nameElf = elfSym.st_name;

        inputFile.symbolTable.push_back(symRow);
      }
    } else if(shdr.sh_type == SHT_RELA) {
      size_t sectionIndex = i;
      for(auto &section : inputFile.sections) {
        if(shdr.sh_info == section.secIndexElf) {
          ifs.seekg(shdr.sh_offset, ios::beg);
          size_t numRelaEntries = shdr.sh_size / sizeof(Elf64_Rela);
          section.relaTable.resize(numRelaEntries);
          for(size_t j = 0; j < numRelaEntries; ++j) {
            Elf64_Rela elfRela;
            ifs.read(reinterpret_cast<char*>(&elfRela), sizeof(elfRela));
            if(!ifs) {
              cerr << "Failed to read relocation entry." << endl;
              return;
            }

            relaTableRow relaRow;
            relaRow.offset = elfRela.r_offset;
            relaRow.type = (ELF64_R_TYPE(elfRela.r_info) == R_X86_64_32S) ? MY_R_X86_64_32S : MY_R_X86_64_PC32;
            relaRow.symbol = ELF64_R_SYM(elfRela.r_info);
            relaRow.addend = elfRela.r_addend;

            section.relaTable[j] = relaRow;
          }
          break;
        }
      }
    }
  }

  // Read strtab and update symbol names
  Elf64_Shdr lastStrtabShdr;

  for(const auto &shdr : shdrs) {
    if(shdr.sh_type == SHT_STRTAB) {
      lastStrtabShdr = shdr;
    }
  }

  string strtab;
  ifs.seekg(lastStrtabShdr.sh_offset, ios::beg);
  strtab.resize(lastStrtabShdr.sh_size);
  ifs.read(&strtab[0], strtab.size());
  if(!ifs) {
    cerr << "Failed to read string table." << endl;
    return;
  }

  for(auto &sym : inputFile.symbolTable) {
    if(sym.name.empty() && sym.num < strtab.size()) {
      sym.name = strtab.c_str() + sym.nameElf;
    }
  }

  for(auto &section : inputFile.sections) {
    for(auto &sym : inputFile.symbolTable) {
      if(section.sectionName == sym.name) {
        section.sectionNum = sym.num;
      }
    }
  }

  inputFilesSections.push_back(inputFile);

  ifs.close();
}


void Linker::mapping() {

}

void Linker::symbolDetermination() {

}

void Linker::symbolResolution() {

}


void Linker::printSymbolTable(const vector<symbolTableRow>& symbolTable) {
  cout << left
    << setw(10) << "Num"
    << setw(10) << "Value"
    << setw(10) << "Type"
    << setw(10) << "Bind"
    << setw(15) << "SectionIndex"
    << setw(20) << "Name"
    << endl;
  
  cout << string(75, '-') << endl;

  for(const auto& row : symbolTable) {
    cout << left
      << setw(10) << dec << row.num;
    printf("%08X  ", row.value);
    cout << setw(10) << (row.type == NOTYP ? "NOTYP" : "SCTN")
      << setw(10) << (row.bind == LOC ? "LOC" : "GLOB")
      << setw(15) << dec << row.sectionIndex
      << setw(20) << row.name
      << endl;
  }
  cout << endl;
}

void Linker::printRelaTables(const sectionStruct& section) {
  cout << "Relocation Table:" << endl;

  cout << left
    << setw(10) << "Offset"
    << setw(20) << "Type"
    << setw(10) << "Symbol"
    << setw(10) << "Addend"
    << endl;
  
  cout << string(50, '-') << endl;

  for(const auto& row : section.relaTable) {
    cout << left;
    printf("%08X  ", row.offset);
    cout << setw(20) << (row.type == MY_R_X86_64_32S ? "MY_R_X86_64_32S" : "MY_R_X86_64_PC32")
      << setw(10) << dec << row.symbol
      << setw(10) << dec << row.addend
      << endl;
  }
}

void Linker::printSections(const vector<sectionStruct>& sections) {
  for(const auto& section : sections) {
    cout << "Section Number: " << section.sectionNum << endl;
    cout << "Section Name: " << section.sectionName << endl;
    cout << "Section Data (8-bit values): ";
    for(const auto& val : section.sectionData8bitValues) {
      printf("%02X ", val);
    }
    cout << endl;
    cout << "Location Counter: " << section.locationCounter << endl;
    printRelaTables(section);
    cout << endl << endl;
  }
  cout << endl;
}

void Linker::printInputFilesSections() {
  for (const auto& file : inputFilesSections) {
    cout << "Input file name: " << file.inputFileName << endl;
    cout << "Symbol table: " << endl;
    printSymbolTable(file.symbolTable);
    cout << endl;
    cout << "File sections: " << endl << endl;
    printSections(file.sections);
    cout << endl;
  }
}
