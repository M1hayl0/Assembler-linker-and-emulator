#include "./../inc/emulator.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <chrono>

Emulator::Emulator(char *inputFile) : gprs(16, 0), csrs(3, 0) {
  inputFileName = string(inputFile);
  gprs[15] = 0x40000000;

  timerThread = thread(&Emulator::emulatingTimer, this);

  sem_init(&mutex, 0, 1);
}

Emulator::~Emulator() {
  end = true;
  if(timerThread.joinable()) timerThread.join();

  sem_destroy(&mutex);
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

  setRawMode(true);
  setNonBlocking(true);
  
  while(!end) {
    sem_wait(&mutex);
    
    emulatingTerminal();

    instruction = read4Bytes(gprs[15]);
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
    sem_post(&mutex);
  }

  setRawMode(false);
  setNonBlocking(false);
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
  if(!(csrs[0] & 0x4)) {
    // push status; push pc; cause<=4; status<=status | 0x4; pc<=handler; 
    gprs[14] = gprs[14] - 4;
    write4Bytes(gprs[14], csrs[0]);

    gprs[14] = gprs[14] - 4;
    write4Bytes(gprs[14], gprs[15]);

    csrs[2] = 4;

    csrs[0] |= 0x4;

    gprs[15] = csrs[1];
  }
}

void Emulator::call(uint8_t MOD, uint8_t A, uint8_t B, int32_t D) {
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
  if(B == 0 || C == 0) return;
  uint32_t temp = gprs[B];
  gprs[B] = gprs[C];
  gprs[C] = temp;
}

void Emulator::arit(uint8_t MOD, uint8_t A, uint8_t B, uint8_t C) {
  if(A == 0) return;
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
  if(A == 0) return;
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
  if(A == 0) return;
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
  switch(MOD) {
    case 0:
      write4Bytes(gprs[A] + gprs[B] + D, gprs[C]);
      break;
    case 1:
      if(A == 0) return;
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
  switch(MOD) {
    case 0:
      if(A == 0) return;
      gprs[A] = csrs[B];
      break;
    case 1:
      if(A == 0) return;
      gprs[A] = gprs[B] + D;
      break;
    case 2:
      if(A == 0) return;
      gprs[A] = read4Bytes(gprs[B] + gprs[C] + D) ;
      break;
    case 3:
      if(A == 0 || B == 0) return;
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
      if(B == 0) return;
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


void Emulator::emulatingTerminal() {
  if(memory.find(TERM_OUT_START) != memory.end()) {
    uint8_t value = memory[TERM_OUT_START];
    cout << static_cast<char>(value);
    cout.flush();
    memory.erase(TERM_OUT_START);
  }

  if(!(csrs[0] & 0x2) && !(csrs[0] & 0x4)) {
    char input;
    if(read(STDIN_FILENO, &input, 1) > 0) {
      memory[TERM_IN_START] = static_cast<uint8_t>(input);
      
      // push status; push pc; cause<=3; status<=status | 0x2; pc<=handler; 
      gprs[14] = gprs[14] - 4;
      write4Bytes(gprs[14], csrs[0]);

      gprs[14] = gprs[14] - 4;
      write4Bytes(gprs[14], gprs[15]);

      csrs[2] = 3;

      csrs[0] |= 0x2;

      gprs[15] = csrs[1];
    }
  }
}

void Emulator::setRawMode(bool enable) {
  static struct termios oldt, newt;
  if(enable) {
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  } else {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  }
}

void Emulator::setNonBlocking(bool enable) {
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  if(flags == -1) return;
  if(enable) fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
  else fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}


void Emulator::emulatingTimer() {
  while(!end) {
    sem_wait(&mutex);
    if(memory.find(TIM_CFG_START) != memory.end()) {
      if(!(csrs[0] & 0x1) && !(csrs[0] & 0x4)) {
        int tim_cfg_value = memory[TIM_CFG_START];
        int period_ms = getTimerPeriod(tim_cfg_value);

        if(period_ms > 0) {
          sem_post(&mutex);
          this_thread::sleep_for(chrono::milliseconds(period_ms));
          sem_wait(&mutex);

          // push status; push pc; cause<=2; status<=status | 0x1; pc<=handler; 
          gprs[14] = gprs[14] - 4;
          write4Bytes(gprs[14], csrs[0]);

          gprs[14] = gprs[14] - 4;
          write4Bytes(gprs[14], gprs[15]);

          csrs[2] = 2;

          csrs[0] |= 0x1;

          gprs[15] = csrs[1];
        }
      }
    }
    sem_post(&mutex);
  }
}

int Emulator::getTimerPeriod(int tim_cfg_value) {
  switch(tim_cfg_value) {
    case 0x0: return 500;
    case 0x1: return 1000;
    case 0x2: return 1500;
    case 0x3: return 2000;
    case 0x4: return 5000;
    case 0x5: return 10000;
    case 0x6: return 30000;
    case 0x7: return 60000;
    default: return -1;
  }
}