#ifndef EMULATOR_H
#define EMULATOR_H

#include <string>
#include <vector>

using namespace std;

class Emulator {
private:
  vector<uint8_t> hexFile;

  string inputFileName;
public:
  Emulator(char *);
  void emulate();

  void hexRead();
};

#endif // EMULATOR_H