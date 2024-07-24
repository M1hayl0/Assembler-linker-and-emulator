#include <stdio.h>

#include "argumentTrasfer.h"
#include "lexer.h"
#include "parser.h"

#include "assembler.hpp"


int main(int argc, char* argv[]) {
    if (yyparse()) return 1;
    // printLines(head);

    Assembler *assembler = new Assembler(head);
    assembler->assemble();
    delete assembler;

    freeLines(head);
    return 0;
}