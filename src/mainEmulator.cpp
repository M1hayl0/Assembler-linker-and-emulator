#include <stdio.h>
#include <iostream>

#include "./../inc/emulator.hpp"

int main(int argc, char* argv[]) {
  if(argc < 2 || string(argv[0]) != "./../../build/emulator") {
    cout << "Call program like this: ./../../build/emulator <input_file>\n" << endl;
    return 1;
  }

  FILE *f = fopen(argv[1], "r");
  if(!f) {
    cout << "Opening file error" << endl;
    return 1;
  }

  Emulator *emulator = new Emulator(argv[1]);
  emulator->emulate();
  delete emulator;

  return 0;
}