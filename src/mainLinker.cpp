#include <stdio.h>
#include <iostream>
#include <map>
#include <vector>
#include <cstring>

#include "./../inc/linker.hpp"

bool parseArgs(int argc, char *argv[], string &outputFile, map<string, uint> &place, bool &hex, bool &relocatable, vector<string> &inputFiles) {
  for(int i = 1; i < argc; ) {
    if(strcmp(argv[i], "-o") == 0)  {
      outputFile = string(argv[i + 1]);
      i += 2;
    } else if(string(argv[i]).find("-place=") == 0) {
      string value = string(argv[i]).substr(7);
      int atPos = value.find("@");

      if(atPos != string::npos) {
        string sectionName = value.substr(0, atPos);
        string addressStr = value.substr(atPos + 1);
        uint address = 0;
        if (addressStr.find("0x") == 0 || addressStr.find("0X") == 0) {
          address = static_cast<uint>(stoul(addressStr, nullptr, 16));
        } else {
          address = static_cast<uint>(stoul(addressStr));
        }

        place[sectionName] = address;
      } else {
        cout << "Invalid -place argument format: " << argv[i] << endl;
        return false;
      }

      i++;
    } else if(strcmp(argv[i], "-hex") == 0) {
      hex = true;
      i++;
    } else if(strcmp(argv[i], "-relocatable") == 0) {
      relocatable = true;
      i++;
    } else {
      while(i < argc) {
        if(strcmp(argv[i], "-o") == 0 || string(argv[i]).find("-place=") == 0 || strcmp(argv[i], "-hex") == 0 || strcmp(argv[i], "-relocatable") == 0) return false;
        inputFiles.push_back(string(argv[i]));
        i++;
      }
    }
  }
  return true;
}

int main(int argc, char* argv[]) {
  string outputFile;
  map<string, uint> place;
  bool hex = false;
  bool relocatable = false;
  vector<string> inputFiles;

  if(string(argv[0]) != "./../../build/linker") {
    cout << "Call program like this: ./../../build/linker[-hex or -relocatable] -place=code@0x0200 -place=code2@0x0400 -o <output_file> <input_files>\n" << endl;
    return 1;
  }
  
  if(!parseArgs(argc, argv, outputFile, place, hex, relocatable, inputFiles)) {
    cout << "Call program like this: ./../../build/linker [-hex or -relocatable] -place=code@0x0200 -place=code2@0x0400 -o <output_file> <input_files>\n" << endl;
    return 1;
  }

  if((!hex && !relocatable) || (hex && relocatable)) {
    cout << "There must be one -hex or one -relocatable" << endl;
    return 1;
  }

  Linker *linker = new Linker(outputFile, place, hex, relocatable, inputFiles);
  linker->link();
  delete linker;

  return 0;
}