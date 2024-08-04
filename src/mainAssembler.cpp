#include <stdio.h>
#include <iostream>

#include "./../inc/argumentTrasfer.h"
#include "./../build/lexer.h"
#include "./../build/parser.h"

#include "./../inc/assembler.hpp"


int main(int argc, char* argv[]) {
  if(argc < 4 || string(argv[0]) != "./../../build/assembler" || string(argv[1]) != "-o") {
    cout << "Call program like this: ./../../build/assembler -o <output_file> <input_file>\n" << endl;
    return 1;
  }

  FILE *f = fopen(argv[3], "r");
  if(!f) {
    cout << "Opening file error" << endl;
    return 1;
  }

  yyin = f;
  if(yyparse()) return 1;
  fclose(f);

  // printLines(head);

  Assembler *assembler = new Assembler(head, argv[2]);
  assembler->assemble();
  delete assembler;

  freeLines(head);
  return 0;
}