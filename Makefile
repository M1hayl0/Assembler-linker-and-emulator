test: all
	cat main.s | ./main

all: main.cpp helpers.c parser.c lexer.c helpers.h
	g++ main.cpp helpers.c parser.c lexer.c -o main

lexer.c: lexer.l helpers.h
	flex lexer.l

parser.c: parser.y lexer.l helpers.h
	bison parser.y

clean:
	rm -rf *.o lexer.c lexer.h parser.c parser.h main