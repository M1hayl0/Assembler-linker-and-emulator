#ifndef EMULATOR_H
#define EMULATOR_H

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <thread>

using namespace std;

class Emulator {
private:
  string inputFileName;


  map<uint32_t, uint8_t> memory;
  vector<uint32_t> gprs;
  vector<uint32_t> csrs;


  atomic<bool> end;
  thread timerThread;

  const uint32_t TERM_OUT_START = 0xFFFFFF00;
  const uint32_t TERM_OUT_END = 0xFFFFFF03;
  const uint32_t TERM_IN_START = 0xFFFFFF04;
  const uint32_t TERM_IN_END = 0xFFFFFF07;

  const uint32_t TIM_CFG_START = 0xFFFFFF10;
  const uint32_t TIM_CFG_END = 0xFFFFFF13;

public:
  Emulator(char *);
  ~Emulator();
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

  void emulatingTerminal();
  void setRawMode(bool);
  void setNonBlocking(bool);

  void emulatingTimer();
  int getTimerPeriod(uint8_t);
};

#endif // EMULATOR_H