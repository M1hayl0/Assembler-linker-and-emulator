#include <stdio.h>

#include "argumentTrasfer.h"
#include "lexer.h"
#include "parser.h"

#include "assembler.hpp"


int main(int argc, char* argv[]) {
    if(argc < 4 || string(argv[1]) != "-o") {
        printf("Call program like this: %s -o <output_file> <input_file>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[3], "r");
    if(!f) {
        printf("Opening file error");
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