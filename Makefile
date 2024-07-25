test: all
	./main -o output.o test.s
	readelf -a output.o

all: main.cpp argumentTrasfer.c parser.c lexer.c assembler.cpp argumentTrasfer.h assembler.hpp
	g++ main.cpp argumentTrasfer.c parser.c lexer.c assembler.cpp -o main

lexer.c: lexer.l argumentTrasfer.h
	flex lexer.l

parser.c: parser.y lexer.l argumentTrasfer.h
	bison parser.y

clean:
	rm -rf *.o lexer.c lexer.h parser.c parser.h main