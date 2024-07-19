#include "argumentTrasfer.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (yyparse()) return 1;
    printLines(head);
    freeLines(head);
    return 0;
}