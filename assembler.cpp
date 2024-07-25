#include "assembler.hpp"
#include "argumentTrasfer.h"

#include <string.h>
#include <elf.h>
#include <fstream>
#include <iostream>
#include <cstring>


Assembler::Assembler(struct line *lines, char *outputFile) {
  code = lines;
  outputFileName = string(outputFile);
  symbolTable.push_back({0, 0, NOTYP, LOC, 0, "", true, nullptr});
}

void Assembler::assemble() {
  for(struct line *currentLine = code; currentLine; currentLine = currentLine->next) {
    if(currentLine->directive) {
      if(strcmp(currentLine->directive->name, ".global") == 0) {
        globalAssemble(currentLine->directive);
      } else if (strcmp(currentLine->directive->name, ".extern") == 0) {
        externAssemble(currentLine->directive);
      } else if (strcmp(currentLine->directive->name, ".section") == 0) {
        sectionAssemble(currentLine->directive);
      } else if (strcmp(currentLine->directive->name, ".word") == 0) {
        wordAssemble(currentLine->directive);
      } else if (strcmp(currentLine->directive->name, ".skip") == 0) {
        skipAssemble(currentLine->directive);
      } else if (strcmp(currentLine->directive->name, ".ascii") == 0) {
        asciiAssemble(currentLine->directive);
      } else if (strcmp(currentLine->directive->name, ".equ") == 0) {
        equAssemble(currentLine->directive);
      } else if (strcmp(currentLine->directive->name, ".end") == 0) {
        break;
      }
    } else if(currentLine->instruction) {
      if (strcmp(currentLine->instruction->name, "halt") == 0) {
        haltAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "int") == 0) {
        intAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "iret") == 0) {
        iretAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "call") == 0) {
        callAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "ret") == 0) {
        retAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "jmp") == 0) {
        jmpAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "beq") == 0) {
        beqBneBgtAssemble(currentLine->instruction, 0x9);
      } else if (strcmp(currentLine->instruction->name, "bne") == 0) {
        beqBneBgtAssemble(currentLine->instruction, 0xA);
      } else if (strcmp(currentLine->instruction->name, "bgt") == 0) {
        beqBneBgtAssemble(currentLine->instruction, 0xB);
      } else if (strcmp(currentLine->instruction->name, "push") == 0) {
        pushAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "pop") == 0) {
        popAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "xchg") == 0) {
        xchgAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "add") == 0) {
        aritLogShiftAssemble(currentLine->instruction, 0x5, 0x0);
      } else if (strcmp(currentLine->instruction->name, "sub") == 0) {
        aritLogShiftAssemble(currentLine->instruction, 0x5, 0x1);
      } else if (strcmp(currentLine->instruction->name, "mul") == 0) {
        aritLogShiftAssemble(currentLine->instruction, 0x5, 0x2);
      } else if (strcmp(currentLine->instruction->name, "div") == 0) {
        aritLogShiftAssemble(currentLine->instruction, 0x5, 0x3);
      } else if (strcmp(currentLine->instruction->name, "not") == 0) {
        notAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "and") == 0) {
        aritLogShiftAssemble(currentLine->instruction, 0x6, 0x1);
      } else if (strcmp(currentLine->instruction->name, "or") == 0) {
        aritLogShiftAssemble(currentLine->instruction, 0x6, 0x2);
      } else if (strcmp(currentLine->instruction->name, "xor") == 0) {
        aritLogShiftAssemble(currentLine->instruction, 0x6, 0x3);
      } else if (strcmp(currentLine->instruction->name, "shl") == 0) {
        aritLogShiftAssemble(currentLine->instruction, 0x7, 0x0);
      } else if (strcmp(currentLine->instruction->name, "shr") == 0) {
        aritLogShiftAssemble(currentLine->instruction, 0x7, 0x1);
      } else if (strcmp(currentLine->instruction->name, "ld") == 0) {
        ldAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "st") == 0) {
        stAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "csrrd") == 0) {
        csrrdAssemble(currentLine->instruction);
      } else if (strcmp(currentLine->instruction->name, "csrwr") == 0) {
        csrwrAssemble(currentLine->instruction);
      }
    } else if(currentLine->label) {
      labelAssemble(currentLine->label);
    }
  }

  printSymbolTable();
  printSections();
  
  elf();
}


void Assembler::globalAssemble(struct directive *directive) {
  bool found = false;

  for(operandArgs *operand = directive->operands; operand; operand = operand->next) {
    for(auto &row : symbolTable) {
      if(row.name == string(operand->symbol)) {
        found = true;
        row.bind = GLOB;
        break;
      }
    }

    if(!found) {
      symbolTable.push_back({(int) symbolTable.size(), 0, NOTYP, GLOB, 0, string(operand->symbol), false, nullptr});
    }
  }
}

void Assembler::externAssemble(struct directive *directive) {
  bool found = false;

  for(operandArgs *operand = directive->operands; operand; operand = operand->next) {
    for(auto &row : symbolTable) {
      if(row.name == string(operand->symbol) && !row.defined) {
        found = true;
        row.bind = GLOB;
        row.sectionIndex = 0;
        break;
      } else if(row.name == string(operand->symbol) && row.defined) {
        printf("EXTERN SYMBOL CAN'T BE DEFINED HERE");
        exit(0);
      }
    }

    if(!found) {
      symbolTable.push_back({(int) symbolTable.size(), 0, NOTYP, GLOB, 0, string(operand->symbol), false, nullptr});
    }
  }
}

void Assembler::sectionAssemble(struct directive *directive) {
  symbolTable.push_back({(int) symbolTable.size(), 0, SCTN, LOC, (int) sections.size() + 1, string(directive->operands->symbol), true, nullptr});
  sections.push_back({(int) symbolTable.size() - 1, string(directive->operands->symbol), vector<uint8_t>(), 0, vector<relaTableRow>(), (int) sections.size() + 1});
}

void Assembler::wordAssemble(struct directive *directive) {
  sectionStruct &currentSection = sections.back();
  for(operandArgs *operand = directive->operands; operand; operand = operand->next) {
    if(operand->type == symType) {
      bool found = false;
      for(auto &row : symbolTable) {
        if(row.name == string(operand->symbol) && row.defined) {
          found = true;

          int symbol = 0;
          int addend = 0;
          if(row.bind == LOC) {
            symbol = sections[row.sectionIndex - 1].sectionNum;
            addend = row.value;
          } else if(row.bind == GLOB) {
            symbol = row.num;
          }
          currentSection.relaTable.push_back({currentSection.locationCounter, MY_R_X86_64_32S, symbol, addend});

          push32BitValue(0, currentSection);
          break;
        } else if(row.name == string(operand->symbol) && !row.defined) {
          found = true;
          forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter, row.head};
          row.head = refsList;
          push32BitValue(0, currentSection);
          break;
        }
      }
      
      if(!found) {
        forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter, nullptr};
        symbolTable.push_back({(int) symbolTable.size(), 0, NOTYP, LOC, 0, string(operand->symbol), false, refsList});
        push32BitValue(0, currentSection);
      }
    } else if(operand->type == litType) {
      push32BitValue(operand->literal, currentSection);
    }
  }
}

void Assembler::skipAssemble(struct directive *directive) {
  sectionStruct &currentSection = sections.back();
  for(int i = 0; i < directive->operands->literal; i++) currentSection.sectionData8bitValues.push_back(0);
  currentSection.locationCounter += directive->operands->literal;
}

void Assembler::asciiAssemble(struct directive *directive) {
  sectionStruct &currentSection = sections.back();
  for(char *ptr = directive->string; *ptr; ptr++) {
    currentSection.sectionData8bitValues.push_back(*ptr);
    currentSection.locationCounter++;
  }
}

void Assembler::equAssemble(struct directive *directive) {}


void Assembler::haltAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();
  int instructionCode = makeInstructionCode(0, 0, 0, 0, 0, 0, 0, 0);
  push32BitValue(instructionCode, currentSection);
}

void Assembler::intAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();
  int instructionCode = makeInstructionCode(1, 0, 0, 0, 0, 0, 0, 0);
  push32BitValue(instructionCode, currentSection);
}

void Assembler::iretAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();
  // iret-> pop pc, pop status -> pc <= mem32[sp]; sp <= sp + 4; status <= mem32[sp]; sp <= sp + 4;
  int instructionCode = makeInstructionCode(9, 3, 15, 14, 0, 0, 0, 4);
  push32BitValue(instructionCode, currentSection);
  instructionCode = makeInstructionCode(9, 7, 0, 14, 0, 0, 0, 4);
  push32BitValue(instructionCode, currentSection);
}

void Assembler::callAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();

  if(instruction->operand1->type == symJumpType) {
    // call sym -> push pc; pc<=sym; -> pc<=[pc+4] (pc=r15)
    bool found = false;
    for(auto &row : symbolTable) {
      if(row.name == string(instruction->operand1->symbol) && row.defined) {
        found = true;

        int symbol = 0;
        int addend = 0;
        if(row.bind == LOC) {
          symbol = sections[row.sectionIndex - 1].sectionNum;
          addend = 4 + row.value; // 4 because offset is 4 locations after pc
        } else if(row.bind == GLOB) {
          symbol = row.num;
          addend = 4;
        }
        currentSection.relaTable.push_back({currentSection.locationCounter + 8, MY_R_X86_64_PC32, symbol, addend});

        int instructionCode = makeInstructionCode(2, 1, 15, 0, 0, 0, 0, 4); // call [pc+4]
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      } else if(row.name == string(instruction->operand1->symbol) && !row.defined) {
        found = true;
        forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 8, row.head};
        row.head = refsList;
        int instructionCode = makeInstructionCode(2, 1, 15, 0, 0, 0, 0, 4); // call [pc+4]
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      }
    }
    
    if(!found) {
      forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 8, nullptr};
      symbolTable.push_back({(int) symbolTable.size(), 0, NOTYP, LOC, 0, string(instruction->operand1->symbol), false, refsList});
      int instructionCode = makeInstructionCode(2, 1, 15, 0, 0, 0, 0, 4); // call [pc+4]
      push32BitValue(instructionCode, currentSection);
      instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
      push32BitValue(instructionCode, currentSection);
      push32BitValue(0, currentSection); // 32bit symbol
    }
  } else if(instruction->operand1->type == litJumpType) {
    // call lit -> push pc; pc<=lit; -> pc<=[pc+4] (pc=r15)
    int instructionCode = makeInstructionCode(2, 1, 15, 0, 0, 0, 0, 4); // call [pc+4]
    push32BitValue(instructionCode, currentSection);
    instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
    push32BitValue(instructionCode, currentSection);
    push32BitValue(instruction->operand1->literal, currentSection); // 32bit literal
  }
}

void Assembler::retAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();
  // ret -> pop pc -> pc <= mem32[sp]; sp <= sp + 4;
  int instructionCode = makeInstructionCode(9, 3, 15, 14, 0, 0, 0, 4);
  push32BitValue(instructionCode, currentSection);
}

void Assembler::jmpAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();

  if(instruction->operand1->type == symJumpType) {
    // jmp sym -> pc<=sym -> pc<=[pc] (pc=r15)
    bool found = false;
    for(auto &row : symbolTable) {
      if(row.name == string(instruction->operand1->symbol) && row.defined) {
        found = true;

        int symbol = 0;
        int addend = 0;
        if(row.bind == LOC) {
          symbol = sections[row.sectionIndex - 1].sectionNum;
          addend = 0 + row.value; // 0 because pc is same as offset
        } else if(row.bind == GLOB) {
          symbol = row.num;
          addend = 0;
        }
        currentSection.relaTable.push_back({currentSection.locationCounter + 4, MY_R_X86_64_PC32, symbol, addend});

        int instructionCode = makeInstructionCode(3, 8, 15, 0, 0, 0, 0, 0); // jmp [pc]
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      } else if(row.name == string(instruction->operand1->symbol) && !row.defined) {
        found = true;
        forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 4, row.head};
        row.head = refsList;
        int instructionCode = makeInstructionCode(3, 8, 15, 0, 0, 0, 0, 0); // jmp [pc]
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      }
    }
    
    if(!found) {
      forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 4, nullptr};
      symbolTable.push_back({(int) symbolTable.size(), 0, NOTYP, LOC, 0, string(instruction->operand1->symbol), false, refsList});
      int instructionCode = makeInstructionCode(3, 8, 15, 0, 0, 0, 0, 0); // jmp [pc]
      push32BitValue(instructionCode, currentSection);
      push32BitValue(0, currentSection); // 32bit symbol
    }
  } else if(instruction->operand1->type == litJumpType) {
    // jmp lit -> pc<=lit -> pc<=[pc] (pc=r15)
    int instructionCode = makeInstructionCode(3, 8, 15, 0, 0, 0, 0, 0); // jmp [pc]
    push32BitValue(instructionCode, currentSection);
    push32BitValue(instruction->operand1->literal, currentSection); // 32bit literal
  }
}

void Assembler::beqBneBgtAssemble(struct instruction *instruction, int MOD) {
  sectionStruct &currentSection = sections.back();

  if(instruction->operand3->type == symJumpType) {
    // beq %gpr1, %gpr2, sym -> if(gpr1 == gpr2) pc <= sym -> pc<=[pc+4] (pc=r15)
    bool found = false;
    for(auto &row : symbolTable) {
      if(row.name == string(instruction->operand3->symbol) && row.defined) {
        found = true;

        int symbol = 0;
        int addend = 0;
        if(row.bind == LOC) {
          symbol = sections[row.sectionIndex - 1].sectionNum;
          addend = 4 + row.value; // 4 because offset is 4 locations after pc
        } else if(row.bind == GLOB) {
          symbol = row.num;
          addend = 4;
        }
        currentSection.relaTable.push_back({currentSection.locationCounter + 8, MY_R_X86_64_PC32, symbol, addend});

        int instructionCode = makeInstructionCode(3, MOD, 15, instruction->operand1->regNum, instruction->operand2->regNum, 0, 0, 4); // beq [pc+4]
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      } else if(row.name == string(instruction->operand3->symbol) && !row.defined) {
        found = true;
        forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 8, row.head};
        row.head = refsList;
        int instructionCode = makeInstructionCode(3, MOD, 15, instruction->operand1->regNum, instruction->operand2->regNum, 0, 0, 4); // beq [pc+4]
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
      }
    }
    
    if(!found) {
      forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 8, nullptr};
      symbolTable.push_back({(int) symbolTable.size(), 0, NOTYP, LOC, 0, string(instruction->operand3->symbol), false, refsList});
      int instructionCode = makeInstructionCode(3, MOD, 15, instruction->operand1->regNum, instruction->operand2->regNum, 0, 0, 4); // beq [pc+4]
      push32BitValue(instructionCode, currentSection);
      instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
      push32BitValue(instructionCode, currentSection);
      push32BitValue(0, currentSection); // 32bit symbol
    }
  } else if(instruction->operand3->type == litJumpType) {
    // beq %gpr1, %gpr2, lit -> if(gpr1 == gpr2) pc <= lit -> pc<=[pc+4] (pc=r15)
    int instructionCode = makeInstructionCode(3, MOD, 15, instruction->operand1->regNum, instruction->operand2->regNum, 0, 0, 4); // beq [pc+4]
    push32BitValue(instructionCode, currentSection);
    instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
    push32BitValue(instructionCode, currentSection);
    push32BitValue(instruction->operand3->literal, currentSection); // 32bit literal
  }
}

void Assembler::pushAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();
  // push %gpr -> sp <= sp - 4; mem32[sp] <= gpr;
  int d1 = (-4 >> 8) & 0xF;
  int d2 = (-4 >> 4) & 0xF;
  int d3 = -4 & 0xF;
  int instructionCode = makeInstructionCode(8, 1, 14, 0, instruction->operand1->regNum, d1, d2, d3);
  push32BitValue(instructionCode, currentSection);
}

void Assembler::popAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();
  // pop %gpr -> gpr <= mem32[sp]; sp <= sp + 4;
  int instructionCode = makeInstructionCode(9, 3, instruction->operand1->regNum, 14, 0, 0, 0, 4);
  push32BitValue(instructionCode, currentSection);
}

void Assembler::xchgAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();
  int instructionCode = makeInstructionCode(4, 0, 0, instruction->operand2->regNum, instruction->operand1->regNum, 0, 0, 0);
  push32BitValue(instructionCode, currentSection);
}

void Assembler::notAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();
  int instructionCode = makeInstructionCode(6, 0, instruction->operand1->regNum, instruction->operand1->regNum, 0, 0, 0, 0);
  push32BitValue(instructionCode, currentSection);
}

void Assembler::aritLogShiftAssemble(struct instruction *instruction, int OC, int MOD) {
  sectionStruct &currentSection = sections.back();
  int instructionCode = makeInstructionCode(OC, MOD, instruction->operand2->regNum, instruction->operand2->regNum, instruction->operand1->regNum, 0, 0, 0);
  push32BitValue(instructionCode, currentSection);
}

void Assembler::ldAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();

  if(instruction->operand1->type == valueLitType) {
    int instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, 15, 0, 0, 0, 4); // ld [pc+4], %gpr
    push32BitValue(instructionCode, currentSection);
    instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
    push32BitValue(instructionCode, currentSection);
    push32BitValue(instruction->operand1->literal, currentSection); // 32bit literal
  } else if(instruction->operand1->type == valueSymType) {
    bool found = false;
    for(auto &row : symbolTable) {
      if(row.name == string(instruction->operand1->symbol) && row.defined) {
        found = true;

        int symbol = 0;
        int addend = 0;
        if(row.bind == LOC) {
          symbol = sections[row.sectionIndex - 1].sectionNum;
          addend = row.value;
        } else if(row.bind == GLOB) {
          symbol = row.num;
        }
        currentSection.relaTable.push_back({currentSection.locationCounter + 8, MY_R_X86_64_32S, symbol, addend});

        int instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, 15, 0, 0, 0, 4); // ld [pc+4], %gpr
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      } else if(row.name == string(instruction->operand1->symbol) && !row.defined) {
        found = true;
        forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 8, row.head};
        row.head = refsList;
        int instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, 15, 0, 0, 0, 4); // ld [pc+4], %gpr
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      }
    }
    
    if(!found) {
      forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 8, nullptr};
      symbolTable.push_back({(int) symbolTable.size(), 0, NOTYP, LOC, 0, string(instruction->operand1->symbol), false, refsList});
      int instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, 15, 0, 0, 0, 4); // ld [pc+4], %gpr
      push32BitValue(instructionCode, currentSection);
      instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
      push32BitValue(instructionCode, currentSection);
      push32BitValue(0, currentSection); // 32bit symbol
    }
  } else if(instruction->operand1->type == litType) {
    int instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, 15, 0, 0, 0, 8); // ld [pc+8], %gpr
    push32BitValue(instructionCode, currentSection);
    instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, instruction->operand2->regNum, 0, 0, 0, 0); // ld [gpr], %gpr
    push32BitValue(instructionCode, currentSection);
    instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
    push32BitValue(instructionCode, currentSection);
    push32BitValue(instruction->operand1->literal, currentSection); // 32bit literal
  } else if(instruction->operand1->type == symType) {
    bool found = false;
    for(auto &row : symbolTable) {
      if(row.name == string(instruction->operand1->symbol) && row.defined) {
        found = true;

        int symbol = 0;
        int addend = 0;
        if(row.bind == LOC) {
          symbol = sections[row.sectionIndex - 1].sectionNum;
          addend = row.value;
        } else if(row.bind == GLOB) {
          symbol = row.num;
        }
        currentSection.relaTable.push_back({currentSection.locationCounter + 12, MY_R_X86_64_32S, symbol, addend});

        int instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, 15, 0, 0, 0, 8); // ld [pc+8], %gpr
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, instruction->operand2->regNum, 0, 0, 0, 0); // ld [gpr], %gpr
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      } else if(row.name == string(instruction->operand1->symbol) && !row.defined) {
        found = true;
        forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 12, row.head};
        row.head = refsList;
        int instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, 15, 0, 0, 0, 8); // ld [pc+8], %gpr
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, instruction->operand2->regNum, 0, 0, 0, 0); // ld [gpr], %gpr
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      }
    }
    
    if(!found) {
      forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 12, nullptr};
      symbolTable.push_back({(int) symbolTable.size(), 0, NOTYP, LOC, 0, string(instruction->operand1->symbol), false, refsList});
      int instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, 15, 0, 0, 0, 8); // ld [pc+8], %gpr
      push32BitValue(instructionCode, currentSection);
      instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, instruction->operand2->regNum, 0, 0, 0, 0); // ld [gpr], %gpr
      push32BitValue(instructionCode, currentSection);
      instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
      push32BitValue(instructionCode, currentSection);
      push32BitValue(0, currentSection); // 32bit symbol
    }
  } else if(instruction->operand1->type == regType) {
    int instructionCode = makeInstructionCode(9, 1, instruction->operand2->regNum, instruction->operand1->regNum, 0, 0, 0, 0); // ld %gpr, %gpr
    push32BitValue(instructionCode, currentSection);
  } else if(instruction->operand1->type == regMemType) {
    int instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, instruction->operand1->regNum, 0, 0, 0, 0); // ld [%gpr], %gpr
    push32BitValue(instructionCode, currentSection);
  } else if(instruction->operand1->type == regMemLitType) {
    if(instruction->operand1->literal > 0xFFF) {
      printf("MAX LITERAL FOR THIS ADDRESSING IS 0xFFF\n");
      exit(0);
    }
    int d1 = (instruction->operand1->literal >> 8) & 0xF;
    int d2 = (instruction->operand1->literal >> 4) & 0xF;
    int d3 = instruction->operand1->literal & 0xF;
    int instructionCode = makeInstructionCode(9, 2, instruction->operand2->regNum, instruction->operand1->regNum, 0, d1, d2, d3); // ld [%gpr + lit], %gpr
    push32BitValue(instructionCode, currentSection);
  } else if(instruction->operand1->type == regMemSymType) {
    printf("SYMBOL VALUE IS UNKNOWN\n"); // change this later for equ
    exit(0);
  }
}

void Assembler::stAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();

  if(instruction->operand2->type == valueLitType) {
    int instructionCode = makeInstructionCode(8, 0, 15, 0, instruction->operand1->regNum, 0, 0, 4); // st %gpr, [pc+4]
    push32BitValue(instructionCode, currentSection);
    instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
    push32BitValue(instructionCode, currentSection);
    push32BitValue(instruction->operand2->literal, currentSection); // 32bit literal
  } else if(instruction->operand2->type == valueSymType) {
    bool found = false;
    for(auto &row : symbolTable) {
      if(row.name == string(instruction->operand2->symbol) && row.defined) {
        found = true;

        int symbol = 0;
        int addend = 0;
        if(row.bind == LOC) {
          symbol = sections[row.sectionIndex - 1].sectionNum;
          addend = row.value;
        } else if(row.bind == GLOB) {
          symbol = row.num;
        }
        currentSection.relaTable.push_back({currentSection.locationCounter + 8, MY_R_X86_64_32S, symbol, addend});

        int instructionCode = makeInstructionCode(8, 0, 15, 0, instruction->operand1->regNum, 0, 0, 4); // st %gpr, [pc+4]
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      } else if(row.name == string(instruction->operand2->symbol) && !row.defined) {
        found = true;
        forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 8, row.head};
        row.head = refsList;
        int instructionCode = makeInstructionCode(8, 0, 15, 0, instruction->operand1->regNum, 0, 0, 4); // st %gpr, [pc+4]
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      }
    }
    
    if(!found) {
      forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 8, nullptr};
      symbolTable.push_back({(int) symbolTable.size(), 0, NOTYP, LOC, 0, string(instruction->operand2->symbol), false, refsList});
      int instructionCode = makeInstructionCode(8, 0, 15, 0, instruction->operand1->regNum, 0, 0, 4); // st %gpr, [pc+4]
      push32BitValue(instructionCode, currentSection);
      instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
      push32BitValue(instructionCode, currentSection);
      push32BitValue(0, currentSection); // 32bit symbol
    }
  } else if(instruction->operand2->type == litType) {
    int instructionCode = makeInstructionCode(8, 2, 15, 0, instruction->operand1->regNum, 0, 0, 4); // st %gpr, [[pc+4]]
    push32BitValue(instructionCode, currentSection);
    instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
    push32BitValue(instructionCode, currentSection);
    push32BitValue(instruction->operand2->literal, currentSection); // 32bit literal
  } else if(instruction->operand2->type == symType) {
    bool found = false;
    for(auto &row : symbolTable) {
      if(row.name == string(instruction->operand2->symbol) && row.defined) {
        found = true;

        int symbol = 0;
        int addend = 0;
        if(row.bind == LOC) {
          symbol = sections[row.sectionIndex - 1].sectionNum;
          addend = row.value;
        } else if(row.bind == GLOB) {
          symbol = row.num;
        }
        currentSection.relaTable.push_back({currentSection.locationCounter + 8, MY_R_X86_64_32S, symbol, addend});

        int instructionCode = makeInstructionCode(8, 2, 15, 0, instruction->operand1->regNum, 0, 0, 4); // st %gpr, [[pc+4]]
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      } else if(row.name == string(instruction->operand2->symbol) && !row.defined) {
        found = true;
        forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 8, row.head};
        row.head = refsList;
        int instructionCode = makeInstructionCode(8, 2, 15, 0, instruction->operand1->regNum, 0, 0, 4); // st %gpr, [[pc+4]]
        push32BitValue(instructionCode, currentSection);
        instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
        push32BitValue(instructionCode, currentSection);
        push32BitValue(0, currentSection); // 32bit symbol
        break;
      }
    }
    
    if(!found) {
      forwardRefsList *refsList = new forwardRefsList{currentSection.locationCounter + 8, nullptr};
      symbolTable.push_back({(int) symbolTable.size(), 0, NOTYP, LOC, 0, string(instruction->operand2->symbol), false, refsList});
      int instructionCode = makeInstructionCode(8, 2, 15, 0, instruction->operand1->regNum, 0, 0, 4); // st %gpr, [[pc+4]]
      push32BitValue(instructionCode, currentSection);
      instructionCode = makeInstructionCode(3, 0, 15, 0, 0, 0, 0, 4); // jmp pc+4
      push32BitValue(instructionCode, currentSection);
      push32BitValue(0, currentSection); // 32bit symbol
    }
  } else if(instruction->operand2->type == regType) {
    int instructionCode = makeInstructionCode(9, 1, instruction->operand2->regNum, instruction->operand1->regNum, 0, 0, 0, 0); // st %gpr, %gpr
    push32BitValue(instructionCode, currentSection);
  } else if(instruction->operand2->type == regMemType) {
    int instructionCode = makeInstructionCode(8, 0, instruction->operand2->regNum, 0, instruction->operand1->regNum, 0, 0, 0); // st %gpr, [%gpr]
    push32BitValue(instructionCode, currentSection);
  } else if(instruction->operand2->type == regMemLitType) {
    if(instruction->operand1->literal > 0xFFF) {
      printf("MAX LITERAL FOR THIS ADDRESSING IS 0xFFF\n");
      exit(0);
    }
    int d1 = (instruction->operand2->literal >> 8) & 0xF;
    int d2 = (instruction->operand2->literal >> 4) & 0xF;
    int d3 = instruction->operand2->literal & 0xF;
    int instructionCode = makeInstructionCode(8, 0, instruction->operand2->regNum, 0, instruction->operand1->regNum, d1, d2, d3); // st %gpr, [%gpr + lit]
    push32BitValue(instructionCode, currentSection);
  } else if(instruction->operand2->type == regMemSymType) {
    printf("SYMBOL VALUE IS UNKNOWN\n"); // change this later for equ
    exit(0);
  }
}

void Assembler::csrrdAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();
  int instructionCode = makeInstructionCode(9, 0, instruction->operand2->regNum, instruction->operand1->regNum - 16, 0, 0, 0, 0); // CSRRD %csr, %gpr
  push32BitValue(instructionCode, currentSection);
}

void Assembler::csrwrAssemble(struct instruction *instruction) {
  sectionStruct &currentSection = sections.back();
  int instructionCode = makeInstructionCode(9, 4, instruction->operand2->regNum - 16, instruction->operand1->regNum, 0, 0, 0, 0); // CSRWR %gpr, %csr
  push32BitValue(instructionCode, currentSection);
}


void Assembler::labelAssemble(struct label *label) {
  sectionStruct &currentSection = sections.back();
  
  bool found = false;
  for(auto &row : symbolTable) {
    if(row.name == string(label->operand->symbol) && !row.defined) {
      found = true;
      row.defined = true;
      row.value = currentSection.locationCounter;
      row.sectionIndex = currentSection.symtabIndex;
      
      for(forwardRefsList *cur = row.head; cur; cur = cur->next) {
        int symbol = 0;
        int addend = 0;
        if(row.bind == LOC) {
          symbol = sections[row.sectionIndex - 1].sectionNum;
          addend = row.value;
        } else if(row.bind == GLOB) {
          symbol = row.num;
        }
        currentSection.relaTable.push_back({cur->patch, MY_R_X86_64_32S, symbol, addend});
      }

      break;
    } else if(row.name == string(label->operand->symbol) && row.defined) {
      printf("TWO SYMBOLS WITH SAME NAME\n");
      exit(0);
    }
  }
  
  if(!found) {
    symbolTable.push_back({(int) symbolTable.size(), currentSection.locationCounter, NOTYP, LOC, currentSection.symtabIndex, string(label->operand->symbol), true, nullptr});
  }
}


void Assembler::push32BitValue(int value, sectionStruct &currentSection) {
  for(int i = 0; i < 4; i++) {
    currentSection.sectionData8bitValues.push_back((value >> (i * 8)) & 0xFF);
  }
  currentSection.locationCounter += 4;
}

int Assembler::makeInstructionCode(int OC, int MOD, int A, int B, int C, int D1, int D2, int D3) {
  int instructionCode = 0x00000000;
  instructionCode |= OC << 7 * 4;
  instructionCode |= MOD << 6 * 4;
  instructionCode |= A << 5 * 4;
  instructionCode |= B << 4 * 4;
  instructionCode |= C << 3 * 4;
  instructionCode |= D1 << 2 * 4;
  instructionCode |= D2 << 1 * 4;
  instructionCode |= D3;
  return instructionCode;
}


void Assembler::printSymbolTable() {
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

void Assembler::printRelaTables(const sectionStruct& section) {
  cout << "Relocation Table:" << endl;

  cout << left
    << setw(10) << "Offset"
    << setw(20) << "Type"
    << setw(10) << "Symbol"
    << setw(10) << "Addend"
    << endl;
  
  cout << string(50, '-') << endl;

  for (const auto& row : section.relaTable) {
    cout << left;
    printf("%08X  ", row.offset);
    cout << setw(20) << (row.type == MY_R_X86_64_32S ? "MY_R_X86_64_32S" : "MY_R_X86_64_PC32")
      << setw(10) << dec << row.symbol
      << setw(10) << dec << row.addend
      << endl;
  }
}

void Assembler::printSections() {
  for (const auto& section : sections) {
    cout << "Section Number: " << section.sectionNum << endl;
    cout << "Section Name: " << section.sectionName << endl;
    cout << "Section Data (8-bit values): ";
    for (const auto& val : section.sectionData8bitValues) {
      printf("%02X ", val);
    }
    cout << endl;
    cout << "Location Counter: " << section.locationCounter << endl;
    printRelaTables(section);
    cout << endl << endl;
  }
  cout << endl;
}


void Assembler::elf() {
  ofstream ofs(outputFileName, ios::binary);
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
  ehdr.e_shnum = sections.size() * 2 + 4;
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

  for(const auto& section : sections) {
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
  for(size_t i = 0; i < sections.size(); i++) {
    shdrs[i + 2].sh_name = section_name_offsets[i];
    shdrs[i + 2].sh_type = SHT_PROGBITS;
    shdrs[i + 2].sh_offset = offset;
    shdrs[i + 2].sh_size = sections[i].sectionData8bitValues.size();
    offset += shdrs[i + 2].sh_size;
  }

  // Relocation table headers
  for(size_t i = 0; i < sections.size(); ++i) {
    shdrs[sections.size() + i + 2].sh_name = rela_name_offsets[i];
    shdrs[sections.size() + i + 2].sh_type = SHT_RELA;
    shdrs[sections.size() + i + 2].sh_offset = offset;
    shdrs[sections.size() + i + 2].sh_size = sections[i].relaTable.size() * sizeof(Elf64_Rela);
    shdrs[sections.size() + i + 2].sh_link = sections.size() * 2 + 2; // Link to .symtab
    shdrs[sections.size() + i + 2].sh_info = i + 2; // Index of the associated section
    shdrs[sections.size() + i + 2].sh_addralign = 8;
    shdrs[sections.size() + i + 2].sh_entsize = sizeof(Elf64_Rela);
    offset += shdrs[sections.size() + i + 2].sh_size;
  }

  // Symtab header
  shdrs[sections.size() * 2 + 2].sh_name = symtab_offset;
  shdrs[sections.size() * 2 + 2].sh_type = SHT_SYMTAB;
  shdrs[sections.size() * 2 + 2].sh_offset = offset;
  shdrs[sections.size() * 2 + 2].sh_size = symbolTable.size() * sizeof(Elf64_Sym);
  shdrs[sections.size() * 2 + 2].sh_link = sections.size() * 2 + 3; // Link to .strtab
  shdrs[sections.size() * 2 + 2].sh_info = symbolTable.size();
  shdrs[sections.size() * 2 + 2].sh_addralign = 8;
  shdrs[sections.size() * 2 + 2].sh_entsize = sizeof(Elf64_Sym);
  offset += shdrs[sections.size() * 2 + 2].sh_size;

  // Strtab header
  shdrs[sections.size() * 2 + 3].sh_name = strtab_offset;
  shdrs[sections.size() * 2 + 3].sh_type = SHT_STRTAB;
  shdrs[sections.size() * 2 + 3].sh_offset = offset;
  shdrs[sections.size() * 2 + 3].sh_size = 1; // initially only null byte
  for(const auto& sym : symbolTable) {
    shdrs[sections.size() * 2 + 3].sh_size += sym.name.size() + 1;
  }

  ofs.write(reinterpret_cast<char*>(shdrs.data()), shdrs.size() * sizeof(Elf64_Shdr));

  // Write shstrtab
  ofs.write(shstrtab.c_str(), shstrtab.size());

  // Write section data
  for(const auto& section : sections) {
    ofs.write(reinterpret_cast<const char*>(section.sectionData8bitValues.data()), section.sectionData8bitValues.size());
  }

  // Write relocation tables
  for(const auto& section : sections) {
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
  for(const auto& sym : symbolTable) {
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