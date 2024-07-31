#include "linker.hpp"

#include <fstream>
#include <iostream>
#include <cstring>
#include <elf.h>
#include <algorithm>
#include <set>
#include <iomanip>

using namespace std;

Linker::Linker(string outputFile, map<string, int> place, bool hexBool, bool relocatableBool, vector<string> inputFiles) {
  this->outputFile = outputFile;
  this->place = place;
  this->hexBool = hexBool;
  this->relocatableBool = relocatableBool;
  this->inputFiles = inputFiles;
}

void Linker::link() {
  for(auto file : inputFiles) elfRead(file);
  // printInputFilesSections();

  mapping();
  symbolDetermination();
  if(hexBool) symbolResolution();
  else if(relocatableBool) makeRelocatableNewSections();

  if(hexBool) hexWrite();
  else if(relocatableBool) elfWrite();
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
  string sectionWithHighestStartAddress;
  int highestStartAddress = 0;
  for(const auto& section : place) {
    if(section.second > highestStartAddress) {
      sectionWithHighestStartAddress = section.first;
      highestStartAddress = section.second;
    }
  }

  notFixedSectionsStartAddress = highestStartAddress;
  for(auto &file : inputFilesSections) {
    for(auto &section : file.sections) {
      if(section.sectionName == sectionWithHighestStartAddress) {
        notFixedSectionsStartAddress += section.locationCounter;
      }
    }
  }

  for(auto &file : inputFilesSections) {
    for(auto &sectionToMap : file.sections) {
      bool found = false;
      for(auto &currentSection : mappedLinkedSections) {
        if(place.find(sectionToMap.sectionName) != place.end()) {
          if(sectionToMap.sectionName == currentSection.sectionName) {
            sectionToMap.addressInLinkedSection = currentSection.size;
            currentSection.size += sectionToMap.locationCounter;
            found = true;
            break;
          }
        } else {
          if(!found && sectionToMap.sectionName == currentSection.sectionName) {
            sectionToMap.addressInLinkedSection = currentSection.size;
            currentSection.size += sectionToMap.locationCounter;
            notFixedSectionsTotalSize += sectionToMap.locationCounter;
            found = true;
          } else if(found) {
            if(place.find(currentSection.sectionName) == place.end()) {
              currentSection.startAddress += sectionToMap.locationCounter;
            }
          }
        }
      }

      if(!found) {
        if(place.find(sectionToMap.sectionName) != place.end()) {
          mappedLinkedSections.push_back({sectionToMap.sectionName, place[sectionToMap.sectionName], sectionToMap.locationCounter});
          sectionToMap.addressInLinkedSection = 0;
        } else {
          mappedLinkedSections.push_back({sectionToMap.sectionName, notFixedSectionsStartAddress + notFixedSectionsTotalSize, sectionToMap.locationCounter});
          sectionToMap.addressInLinkedSection = 0;
          notFixedSectionsTotalSize += sectionToMap.locationCounter;
        }
      }
    }
  }

  vector<mappingStruct> sortedSections = mappedLinkedSections;
  sort(sortedSections.begin(), sortedSections.end(), [](const mappingStruct &a, const mappingStruct &b) {
    return a.startAddress < b.startAddress;
  });

  for(size_t i = 0; i < sortedSections.size() - 1; i++) {
    int endAddress = sortedSections[i].startAddress + sortedSections[i].size;

    if(endAddress > sortedSections[i + 1].startAddress) {
      cout << "OVERLAP FOUND BETWEEN SECTIONS " << sortedSections[i].sectionName
           << " AND " << sortedSections[i + 1].sectionName << endl;
      exit(0);
    }
  }
}

void Linker::symbolDetermination() {
  set<string> symbolNames;
  set<string> sectionNames;
  int lastSectionIndex = 1;
  newSymbolTable.push_back({(int) newSymbolTable.size(), 0, NOTYP, LOC, 0, string()});
  for(auto &file : inputFilesSections) {
    for(auto &symbol : file.symbolTable) {
      if(symbol.num == 0) continue;
      if(symbolNames.find(symbol.name) == symbolNames.end() && sectionNames.find(symbol.name) == sectionNames.end()) {
        if(symbol.type == NOTYP && symbol.bind == GLOB) {
          symbolNames.insert(symbol.name);

          string sectionName;
          for(auto &sec : file.symbolTable) {
            if(sec.type == SCTN && sec.sectionIndex == symbol.sectionIndex) {
              sectionName = sec.name;
              break;
            }
          }

          int addValue = 0;
          for(auto &sec : file.sections) {
            if(sec.sectionName == sectionName) {
              addValue = sec.addressInLinkedSection;
              break;
            }
          }

          newSymbolTable.push_back({(int) newSymbolTable.size(), symbol.value + addValue, symbol.type, symbol.bind, 0, symbol.name, 0, sectionName});
        } else if(symbol.type == SCTN){
          sectionNames.insert(symbol.name);

          int addValue = 0;
          for(auto &sec : file.sections) {
            if(sec.sectionName == symbol.name) {
              addValue = sec.addressInLinkedSection;
            }
          }

          newSymbolTable.push_back({(int) newSymbolTable.size(), symbol.value + addValue, symbol.type, symbol.bind, lastSectionIndex++, symbol.name, 0, symbol.name});
        }
      } else {
        for(auto &symbol2 : newSymbolTable) {
          if(symbol2.bind == GLOB && symbol2.sectionName == "" && symbol.bind == GLOB && symbol.sectionIndex != 0 && symbol.name == symbol2.name) {
            string sectionName;
            for(auto &sec : file.symbolTable) {
              if(sec.type == SCTN && sec.sectionIndex == symbol.sectionIndex) {
                sectionName = sec.name;
                break;
              }
            }

            int addValue = 0;
            for(auto &sec : file.sections) {
              if(sec.sectionName == sectionName) {
                addValue = sec.addressInLinkedSection;
                break;
              }
            }
            
            symbol2.sectionIndex = symbol.sectionIndex;
            symbol2.value = symbol.value + addValue;
          } else if(symbol2.bind == GLOB && symbol2.sectionName != "" && symbol.bind == GLOB && symbol.sectionIndex != 0 && symbol.name == symbol2.name) {
            cout << "GLOBAL SYMBOL DEFINED IN MORE FILES" << endl;
            exit(0);
          }
        }
      }
    }
  }

  for(auto &symbol : newSymbolTable) {
    bool found = false;
    if(symbol.type == NOTYP) {
      for(auto &symbol2 : newSymbolTable) {
        if(symbol2.type == SCTN && symbol.sectionName == symbol2.name) {
          symbol.sectionIndex = symbol2.sectionIndex;
          found = true;
          break;
        }
      }
    }

    if(!found && symbol.type == NOTYP && symbol.num != 0 && symbol.sectionIndex == 0) {
      cout << "UNRESOLVED SYMBOL" << endl;
      exit(0);
    }
  }
  
  if(hexBool) {
    for(auto &symbol : newSymbolTable) {
      for(auto &section : mappedLinkedSections) {
        if(symbol.sectionName == section.sectionName) {
          symbol.value += section.startAddress;
        }
      }
    }
  } else if(relocatableBool) {
    relocatableNewSymbolTable = newSymbolTable;
  }
}

void Linker::symbolResolution() {
  for(auto &file : inputFilesSections) {
    for(auto &section : file.sections) {
      for(auto &relaRow : section.relaTable) {
        int value = 0;
        if(relaRow.type == MY_R_X86_64_32S) value = relaRow.addend;
        else if(relaRow.type == MY_R_X86_64_PC32) value = relaRow.addend - relaRow.offset;

        for(auto &symbol : file.symbolTable) {
          if(symbol.num == relaRow.symbol) {
            for(auto &symbol2 : newSymbolTable) {
              if(symbol2.name == symbol.name) {
                value += symbol2.value;
                break;
              }
            }
            break;
          }
        }

        for(int i = 0; i < 4; i++) {
          section.sectionData8bitValues[relaRow.offset + i] = (value >> (8 * i)) & 0xFF;
        }
      }
    }
  }
}

void Linker::makeRelocatableNewSections() {
  for(auto &symbol : relocatableNewSymbolTable) {
    if(symbol.type == SCTN) {
      relocatableNewSections.push_back({symbol.num, symbol.name, vector<uint8_t>(), 0, vector<relaTableRow>(), symbol.sectionIndex, 0, 0});
    }
  }

  for(auto &file : inputFilesSections) {
    for(auto &section : file.sections) {
      for(auto &section2 : relocatableNewSections) {
        if(section.sectionName == section2.sectionName) {
          section2.sectionData8bitValues.insert(section2.sectionData8bitValues.end(), section.sectionData8bitValues.begin(), section.sectionData8bitValues.end());
          
          for(auto &relaRow : section.relaTable) {
            int newSymbolNum = relaRow.symbol;
            string newSymbolName;
            symbolType newSymbolType;

            for(auto &symbol : file.symbolTable) {
              if(symbol.num == newSymbolNum) {
                newSymbolName = symbol.name;
                newSymbolType = symbol.type;
                break;
              }
            }

            for(auto &symbol : newSymbolTable) {
              if(newSymbolName == symbol.name) {
                newSymbolNum = symbol.num;
                break;
              }
            }

            int addend = relaRow.addend;
            if(newSymbolType == SCTN) addend += section.addressInLinkedSection;

            section2.relaTable.push_back({relaRow.offset + section.addressInLinkedSection, relaRow.type, newSymbolNum, addend});
          }
          break;
        }
      }
    }
  }

}

void Linker::hexWrite() {
  ofstream hexFile(outputFile);
  if (!hexFile.is_open()) {
    cerr << "Error opening file!" << endl;
    return;
  }

  struct hexStruct {
    string sectionName;
    int position;
    vector<uint8_t> sectionData8bitValues;
  };

  vector<hexStruct> memory;
  set<string> sectionNames;

  for(auto &file : inputFilesSections) {
    for(auto &section : file.sections) {
      int position = section.addressInLinkedSection;

      for(auto &section2 : mappedLinkedSections) {
        if(section2.sectionName == section.sectionName) {
          position += section2.startAddress;
          break;
        }
      }

      if(sectionNames.find(section.sectionName) == sectionNames.end()) {
        sectionNames.insert(section.sectionName);
        memory.push_back({section.sectionName, position, section.sectionData8bitValues});
      } else {
        for(auto &section2 : memory) {
          if(section.sectionName == section2.sectionName) {
            if(section2.position + section2.sectionData8bitValues.size() == position) {
              section2.sectionData8bitValues.insert(section2.sectionData8bitValues.end(), section.sectionData8bitValues.begin(), section.sectionData8bitValues.end());
              break;
            } else {
              cout << "ERROR" << endl;
              exit(0);
            }
          }
        }
      }
    }
  }

  sort(memory.begin(), memory.end(), 
    [](const hexStruct& a, 
      const hexStruct& b) {
        return a.position < b.position;
      });

  for (const auto& mem : memory) {
    int i = 0;
    for(uint8_t data : mem.sectionData8bitValues) {
      if(!(i % 8) && i != 0) hexFile << endl;
      if(!(i % 8)) hexFile << hex << setw(8) << setfill('0') << mem.position + i << ": ";
      hexFile << hex << setw(2) << setfill('0') << (int) data << " ";
      i++;
    }
    hexFile << endl << endl;
  }

  hexFile.close();
}

void Linker::elfWrite() {
  ofstream ofs(outputFile, ios::binary);
  if(!ofs) {
    cerr << "Failed to open file for writing." << endl;
    return;
  }

  // ELF Header
  Elf64_Ehdr ehdr;
  std::memset(&ehdr, 0, sizeof(ehdr));
  ehdr.e_ident[EI_MAG0] = ELFMAG0;
  ehdr.e_ident[EI_MAG1] = ELFMAG1;
  ehdr.e_ident[EI_MAG2] = ELFMAG2;
  ehdr.e_ident[EI_MAG3] = ELFMAG3;
  ehdr.e_ident[EI_CLASS] = ELFCLASS64;
  ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
  ehdr.e_ident[EI_VERSION] = EV_CURRENT;
  ehdr.e_ident[EI_OSABI] = ELFOSABI_NONE;
  ehdr.e_type = ET_REL;
  ehdr.e_machine = EM_X86_64;
  ehdr.e_version = EV_CURRENT;
  ehdr.e_phoff = 0;
  ehdr.e_shoff = sizeof(Elf64_Ehdr);
  ehdr.e_flags = 0;
  ehdr.e_ehsize = sizeof(Elf64_Ehdr);
  ehdr.e_phentsize = 0;
  ehdr.e_phnum = 0;
  ehdr.e_shentsize = sizeof(Elf64_Shdr);
  ehdr.e_shnum = relocatableNewSections.size() * 2 + 4;
  ehdr.e_shstrndx = 1;
  
  ofs.write(reinterpret_cast<char*>(&ehdr), sizeof(ehdr));

  // Section Headers
  vector<Elf64_Shdr> shdrs(ehdr.e_shnum);

  // Null section
  memset(&shdrs[0], 0, sizeof(Elf64_Shdr));

  // Section names (shstrtab)
  string shstrtab;
  shstrtab.append("\0", 1);
  vector<size_t> section_name_offsets;
  vector<size_t> rela_name_offsets;

  for(const auto& section : relocatableNewSections) {
    section_name_offsets.push_back(shstrtab.size());
    shstrtab.append(section.sectionName.c_str(), section.sectionName.size() + 1);

    rela_name_offsets.push_back(shstrtab.size());
    shstrtab.append((".rela" + section.sectionName + "\0").c_str(), section.sectionName.size() + 6);
  }

  size_t shstrtab_offset = shstrtab.size();
  shstrtab.append(".shstrtab\0", 10);
  size_t symtab_offset = shstrtab.size();
  shstrtab.append(".symtab\0", 8);
  size_t strtab_offset = shstrtab.size();
  shstrtab.append(".strtab\0", 8);

  // shstrtab section header
  shdrs[1].sh_name = shstrtab_offset;
  shdrs[1].sh_type = SHT_STRTAB;
  shdrs[1].sh_offset = sizeof(Elf64_Ehdr) + shdrs.size() * sizeof(Elf64_Shdr);
  shdrs[1].sh_size = shstrtab.size();
  size_t offset = shdrs[1].sh_offset + shdrs[1].sh_size;

  // Section data header
  for(size_t i = 0; i < relocatableNewSections.size(); i++) {
    shdrs[i + 2].sh_name = section_name_offsets[i];
    shdrs[i + 2].sh_type = SHT_PROGBITS;
    shdrs[i + 2].sh_offset = offset;
    shdrs[i + 2].sh_size = relocatableNewSections[i].sectionData8bitValues.size();
    offset += shdrs[i + 2].sh_size;
  }

  // Relocation table headers
  for(size_t i = 0; i < relocatableNewSections.size(); ++i) {
    shdrs[relocatableNewSections.size() + i + 2].sh_name = rela_name_offsets[i];
    shdrs[relocatableNewSections.size() + i + 2].sh_type = SHT_RELA;
    shdrs[relocatableNewSections.size() + i + 2].sh_offset = offset;
    shdrs[relocatableNewSections.size() + i + 2].sh_size = relocatableNewSections[i].relaTable.size() * sizeof(Elf64_Rela);
    shdrs[relocatableNewSections.size() + i + 2].sh_link = relocatableNewSections.size() * 2 + 2; // Link to .symtab
    shdrs[relocatableNewSections.size() + i + 2].sh_info = i + 2; // Index of the associated section
    shdrs[relocatableNewSections.size() + i + 2].sh_addralign = 8;
    shdrs[relocatableNewSections.size() + i + 2].sh_entsize = sizeof(Elf64_Rela);
    offset += shdrs[relocatableNewSections.size() + i + 2].sh_size;
  }

  // Symtab header
  shdrs[relocatableNewSections.size() * 2 + 2].sh_name = symtab_offset;
  shdrs[relocatableNewSections.size() * 2 + 2].sh_type = SHT_SYMTAB;
  shdrs[relocatableNewSections.size() * 2 + 2].sh_offset = offset;
  shdrs[relocatableNewSections.size() * 2 + 2].sh_size = relocatableNewSymbolTable.size() * sizeof(Elf64_Sym);
  shdrs[relocatableNewSections.size() * 2 + 2].sh_link = relocatableNewSections.size() * 2 + 3; // Link to .strtab
  shdrs[relocatableNewSections.size() * 2 + 2].sh_info = relocatableNewSymbolTable.size();
  shdrs[relocatableNewSections.size() * 2 + 2].sh_addralign = 8;
  shdrs[relocatableNewSections.size() * 2 + 2].sh_entsize = sizeof(Elf64_Sym);
  offset += shdrs[relocatableNewSections.size() * 2 + 2].sh_size;

  // Strtab header
  shdrs[relocatableNewSections.size() * 2 + 3].sh_name = strtab_offset;
  shdrs[relocatableNewSections.size() * 2 + 3].sh_type = SHT_STRTAB;
  shdrs[relocatableNewSections.size() * 2 + 3].sh_offset = offset;
  shdrs[relocatableNewSections.size() * 2 + 3].sh_size = 1; // initially only null byte
  for(const auto& sym : relocatableNewSymbolTable) {
    shdrs[relocatableNewSections.size() * 2 + 3].sh_size += sym.name.size() + 1;
  }

  ofs.write(reinterpret_cast<char*>(shdrs.data()), shdrs.size() * sizeof(Elf64_Shdr));

  // Write shstrtab
  ofs.write(shstrtab.c_str(), shstrtab.size());

  // Write section data
  for(const auto& section : relocatableNewSections) {
    ofs.write(reinterpret_cast<const char*>(section.sectionData8bitValues.data()), section.sectionData8bitValues.size());
  }

  // Write relocation tables
  for(const auto& section : relocatableNewSections) {
    for(const auto& rela : section.relaTable) {
      Elf64_Rela elf_rela;
      elf_rela.r_offset = rela.offset;
      elf_rela.r_info = ELF64_R_INFO(rela.symbol, (rela.type == MY_R_X86_64_32S) ? R_X86_64_32S : R_X86_64_PC32);
      elf_rela.r_addend = rela.addend;
      ofs.write(reinterpret_cast<const char*>(&elf_rela), sizeof(elf_rela));
    }
  }

  // Write symtab
  string strtab;
  strtab.append("\0", 1); // NULL byte
  for(const auto& sym : relocatableNewSymbolTable) {
    Elf64_Sym symbol;
    memset(&symbol, 0, sizeof(symbol));
    symbol.st_name = strtab.size(); // Offset in strtab
    symbol.st_info = ELF64_ST_INFO((sym.bind == GLOB) ? STB_GLOBAL : STB_LOCAL, (sym.type == SCTN) ? STT_SECTION : STT_NOTYPE);
    symbol.st_other = 0;
    symbol.st_shndx = sym.sectionIndex;
    symbol.st_value = sym.value;
    symbol.st_size = 0; // Size is usually set by the linker
    ofs.write(reinterpret_cast<char*>(&symbol), sizeof(symbol));
    strtab.append(sym.name.c_str(), sym.name.size() + 1);
  }

  // Write strtab
  ofs.write(strtab.c_str(), strtab.size());

  ofs.close();
}


void Linker::printSymbolTable(const vector<symbolTableRow> &symbolTable) {
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

void Linker::printRelaTables(const sectionStruct &section) {
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

void Linker::printSections(const vector<sectionStruct> &sections) {
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
