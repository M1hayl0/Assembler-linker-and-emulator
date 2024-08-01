#ifndef EMULATOR_H
#define EMULATOR_H

#include <string>
#include <vector>
#include <map>

using namespace std;

class Emulator {
private:
  string inputFileName;


  map<uint32_t, uint8_t> memory;
  vector<uint32_t> gprs;
  vector<uint32_t> csrs;


public:
  Emulator(char *);
  void emulate();

  void hexRead();
  void emulatingInstructions();
  void printState();

  void interrupt();
  void call(uint8_t, uint8_t, uint8_t, int32_t);
  void jump(uint8_t, uint8_t, uint8_t, uint8_t, int32_t);
  void xchg(uint8_t, uint8_t);
  void arit(uint8_t, uint8_t, uint8_t, uint8_t);
  void log(uint8_t, uint8_t, uint8_t, uint8_t);
  void shift(uint8_t, uint8_t, uint8_t, uint8_t);
  void st(uint8_t, uint8_t, uint8_t, uint8_t, int32_t);
  void ld(uint8_t, uint8_t, uint8_t, uint8_t, int32_t);

  uint32_t read4Bytes(uint32_t);
  void write4Bytes(uint32_t, uint32_t);
};

#endif // EMULATOR_H