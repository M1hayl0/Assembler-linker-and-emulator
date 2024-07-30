test: assemblerAll linkerAll
	./assembler -o output.o test.s
	./assembler -o output2.o test2.s
	./linker -hex -place=code@0x0100 -place=code2@0x0200 -o mem_content.hex output.o output2.o
	./linker -relocatable -place=code@0x0100 -place=code2@0x0200 -o outputLinker.o output.o output2.o
	readelf -a outputLinker.o

assemblerAll: mainAssembler.cpp argumentTrasfer.c parser.c lexer.c assembler.cpp argumentTrasfer.h assembler.hpp
	g++ mainAssembler.cpp argumentTrasfer.c parser.c lexer.c assembler.cpp -o assembler

lexer.c: lexer.l argumentTrasfer.h
	flex lexer.l

parser.c: parser.y lexer.l argumentTrasfer.h
	bison parser.y

linkerAll: mainLinker.cpp linker.cpp linker.hpp
	g++ mainLinker.cpp linker.cpp -o linker

clean:
	rm -rf *.o lexer.c lexer.h parser.c parser.h *.hex assembler linker