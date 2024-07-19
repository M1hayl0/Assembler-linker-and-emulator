test: all
	cat main.s | ./main

all: main.cpp argumentTrasfer.c parser.c lexer.c argumentTrasfer.h
	g++ main.cpp argumentTrasfer.c parser.c lexer.c -o main

lexer.c: lexer.l argumentTrasfer.h
	flex lexer.l

parser.c: parser.y lexer.l argumentTrasfer.h
	bison parser.y

clean:
	rm -rf *.o lexer.c lexer.h parser.c parser.h main