#include "emulator.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <iomanip>

Emulator::Emulator(char *inputFile) : gprs(16, 0), csrs(16, 0) {
  inputFileName = string(inputFile);
  gprs[15] = 0x40000000;
}

void Emulator::emulate() {
  hexRead();
  emulatingInstructions();
  printState();
}

void Emulator::hexRead() {
  ifstream file(inputFileName);

  if(file.is_open()) {
    string line;
    while(getline(file, line)) {
      if(line.empty()) continue;

      stringstream ss(line.substr(0, 10));
      uint32_t address;
      ss >> hex >> address;

      string dataStr = line.substr(10);
      for(int i = 0; i < dataStr.length(); i += 3) {
        string byteStr = dataStr.substr(i, 2);
        uint8_t byte = stoi(byteStr, nullptr, 16);
        memory[address++] = byte;
      }
    }
    file.close();
  } else {
    cout << "Unable to open file";
  }
}

void Emulator::emulatingInstructions() {
  uint32_t instruction;
  bool end = false;
  while(!end) {
    instruction = read4Bytes(gprs[15]);
    cout << "PC " << hex << gprs[15] << endl;
    gprs[15] = gprs[15] + 4;
    uint8_t OC = (instruction >> 28) & 0xF;
    uint8_t MOD = (instruction >> 24) & 0xF;
    uint8_t A = (instruction >> 20) & 0xF;
    uint8_t B = (instruction >> 16) & 0xF;
    uint8_t C = (instruction >> 12) & 0xF;
    int32_t D = instruction & 0xFFF;
    if(D & 0x800) D |= 0xFFFFF000;

    switch(OC) {
      case 0:
        if(instruction == 0) {
          end = true;
          cout << "HALT" << endl;
        } else cout << "UNKNOWN INSTRUCTION" << endl;
        break;
      case 1:
        if(!MOD && !A && !B && !C && !D) interrupt();
        else cout << "UNKNOWN INSTRUCTION" << endl;
        break;
      case 2:
        if(!C) call(MOD, A, B, D);
        else cout << "UNKNOWN INSTRUCTION" << endl;
        break;
      case 3:
        jump(MOD, A, B, C, D);
        break;
      case 4:
        if(!MOD && !A && !D) xchg(B, C);
        else cout << "UNKNOWN INSTRUCTION" << endl;
        break;
      case 5:
        if(!D) arit(MOD, A, B, C);
        else cout << "UNKNOWN INSTRUCTION" << endl;
        break;
      case 6:
        if(!D) log(MOD, A, B, C);
        else cout << "UNKNOWN INSTRUCTION" << endl;
        break;
      case 7:
        if(!D) shift(MOD, A, B, C);
        else cout << "UNKNOWN INSTRUCTION" << endl;
        break;
      case 8:
        st(MOD, A, B, C, D);
        break;
      case 9:
        ld(MOD, A, B, C, D);
        break;
      default:
        cout << "UNKNOWN INSTRUCTION" << endl;
        break;
    }
  }
}

void Emulator::printState() {
  cout << "Emulated processor executed halt instruction\n";
  cout << "Emulated processor state:\n";
  
  for(size_t i = 0; i < gprs.size(); ++i) {
    ostringstream oss;
    oss << "r" << dec << i << "=" << "0x" << setw(8) << setfill('0') << hex << gprs[i];
    string output = oss.str();
    if(i % 4 == 0) cout << setw(14) << setfill(' ') << output;
    else cout << setw(17) << setfill(' ') << output;
    if((i + 1) % 4 == 0) {
      cout << endl;
    }
  }
}


void Emulator::interrupt() {
  // push status; push pc; cause<=4; status<=status&(~0x1); pc<=handle; 
  cout << "INT" << endl;

  gprs[14] = gprs[14] - 4;
  write4Bytes(gprs[14], csrs[0]);

  gprs[14] = gprs[14] - 4;
  write4Bytes(gprs[14], gprs[15]);

  csrs[2] = 4;

  csrs[0] = csrs[0] & (~0x1);

  gprs[15] = csrs[1];
}

void Emulator::call(uint8_t MOD, uint8_t A, uint8_t B, int32_t D) {
  cout << "CALL" << endl;
  switch(MOD) {
    case 0:
      // push pc; pc<=gpr[A]+gpr[B]+D; 
      gprs[14] = gprs[14] - 4;
      write4Bytes(gprs[14], gprs[15]);

      gprs[15] = gprs[A] + gprs[B] + D;
      break;
    case 1:
      // push pc; pc<=mem32[gpr[A]+gpr[B]+D]; 
      gprs[14] = gprs[14] - 4;
      write4Bytes(gprs[14], gprs[15]);

      gprs[15] = read4Bytes(gprs[A] + gprs[B] + D);
      break;
    default:
      cout << "UNKNOWN INSTRUCTION" << endl;
      break;
  }
}

void Emulator::jump(uint8_t MOD, uint8_t A, uint8_t B, uint8_t C, int32_t D) {
  cout << "JUMP" << endl;
  switch(MOD) {
    case 0:
      gprs[15] = gprs[A] + D;
      break;
    case 1:
      if(gprs[B] == gprs[C]) gprs[15] = gprs[A] + D;
      break;
    case 2:
      if(gprs[B] != gprs[C]) gprs[15] = gprs[A] + D;
      break;
    case 3:
      if((int) gprs[B] > (int) gprs[C]) gprs[15] = gprs[A] + D;
      break;
    case 8:
      gprs[15] = read4Bytes(gprs[A] + D);
      break;
    case 9:
      if(gprs[B] == gprs[C]) gprs[15] = read4Bytes(gprs[A] + D);
      break;
    case 10:
      if(gprs[B] != gprs[C]) gprs[15] = read4Bytes(gprs[A] + D);
      break;
    case 11:
      if((int) gprs[B] > (int) gprs[C]) gprs[15] = read4Bytes(gprs[A] + D);
      break;
    default:
      cout << "UNKNOWN INSTRUCTION" << endl;
      break;
  }
}

void Emulator::xchg(uint8_t B, uint8_t C) {
  cout << "XCHG" << endl;
  uint32_t temp = gprs[B];
  gprs[B] = gprs[C];
  gprs[C] = temp;
}

void Emulator::arit(uint8_t MOD, uint8_t A, uint8_t B, uint8_t C) {
  cout << "ARIT" << endl;
  switch(MOD) {
    case 0:
      gprs[A] = gprs[B] + gprs[C];
      break;
    case 1:
      gprs[A] = gprs[B] - gprs[C];
      break;
    case 2:
      gprs[A] = gprs[B] * gprs[C];
      break;
    case 3:
      gprs[A] = gprs[B] / gprs[C];
      break;
    default:
      cout << "UNKNOWN INSTRUCTION" << endl;
      break;
  }
}

void Emulator::log(uint8_t MOD, uint8_t A, uint8_t B, uint8_t C) {
  cout << "LOG" << endl;
  switch(MOD) {
    case 0:
      gprs[A] = ~gprs[B];
      break;
    case 1:
      gprs[A] = gprs[B] & gprs[C];
      break;
    case 2:
      gprs[A] = gprs[B] | gprs[C];
      break;
    case 3:
      gprs[A] = gprs[B] ^ gprs[C];
      break;
    default:
      cout << "UNKNOWN INSTRUCTION" << endl;
      break;
  }
}

void Emulator::shift(uint8_t MOD, uint8_t A, uint8_t B, uint8_t C) {
  cout << "SHIFT" << endl;
  switch(MOD) {
  case 0:
    gprs[A] = gprs[B] << gprs[C];
    break;
  case 1:
    gprs[A] = gprs[B] >> gprs[C];
    break;
  default:
    cout << "UNKNOWN INSTRUCTION" << endl;
    break;
  }
}

void Emulator::st(uint8_t MOD, uint8_t A, uint8_t B, uint8_t C, int32_t D) {
  cout << "ST" << endl;
  switch(MOD) {
    case 0:
      write4Bytes(gprs[A] + gprs[B] + D, gprs[C]);
      break;
    case 1:
      gprs[A] = gprs[A] + (int32_t) D;
      write4Bytes(gprs[A], gprs[C]);
      break;
    case 2:
      write4Bytes(read4Bytes(gprs[A] + gprs[B] + D), gprs[C]);
      break;
    default:
      cout << "UNKNOWN INSTRUCTION" << endl;
      break;
  }
}

void Emulator::ld(uint8_t MOD, uint8_t A, uint8_t B, uint8_t C, int32_t D) {
  cout << "LD" << endl;
  switch(MOD) {
    case 0:
      gprs[A] = csrs[B];
      break;
    case 1:
      gprs[A] = gprs[B] + D;
      break;
    case 2:
      gprs[A] = read4Bytes(gprs[B] + gprs[C] + D) ;
      break;
    case 3:
      gprs[A] = read4Bytes(gprs[B]);
      gprs[B] = gprs[B] + D;
      break;
    case 4:
      csrs[A] = gprs[B];
      break;
    case 5:
      csrs[A] = csrs[B] + D;
      break;
    case 6:
      csrs[A] = read4Bytes(gprs[B] + gprs[C] + D);
      break;
    case 7:
      csrs[A] = read4Bytes(gprs[B]);
      gprs[B] = gprs[B] + D;
      break;
    default:
      cout << "UNKNOWN INSTRUCTION" << endl;
      break;
  }
}


uint32_t Emulator::read4Bytes(uint32_t address) {
  uint32_t value = 0;
  for(int i = 0; i < 4; i++) value |= memory[address + i] << 8 * i;
  return value;
}

void Emulator::write4Bytes(uint32_t address, uint32_t value) {
  for(int i = 0; i < 4; i++) memory[address + i] = static_cast<uint8_t>((value >> 8 * i) & 0xFF);
}