all: assemblerAll linkerAll emulatorAll

test:
	./assemblerDir/assembler -o ./assemblerDir/output.o test.s
	./assemblerDir/assembler -o ./assemblerDir/output2.o test2.s
	./linkerDir/linker -hex -place=code2@0x0100 -o ./linkerDir/mem_content.hex ./assemblerDir/output.o ./assemblerDir/output2.o
	./linkerDir/linker -relocatable -place=code2@0x0100 -place=code@0x0200 -o ./linkerDir/outputLinker.o ./assemblerDir/output.o ./assemblerDir/output2.o
	./emulatorDir/emulator ./linkerDir/mem_content.hex

assemblerAll: assemblerDir/mainAssembler.cpp assemblerDir/argumentTrasfer.c assemblerDir/parser.c assemblerDir/lexer.c assemblerDir/assembler.cpp assemblerDir/argumentTrasfer.h assemblerDir/assembler.hpp
	g++ assemblerDir/mainAssembler.cpp assemblerDir/argumentTrasfer.c assemblerDir/parser.c assemblerDir/lexer.c assemblerDir/assembler.cpp -o assemblerDir/assembler

assemblerDir/lexer.c: assemblerDir/lexer.l assemblerDir/argumentTrasfer.h
	flex -o assemblerDir/lexer.c assemblerDir/lexer.l

assemblerDir/parser.c: assemblerDir/parser.y assemblerDir/lexer.l assemblerDir/argumentTrasfer.h
	bison -o assemblerDir/parser.c assemblerDir/parser.y

linkerAll: linkerDir/mainLinker.cpp linkerDir/linker.cpp linkerDir/linker.hpp
	g++ linkerDir/mainLinker.cpp linkerDir/linker.cpp -o linkerDir/linker

emulatorAll: emulatorDir/mainEmulator.cpp emulatorDir/emulator.cpp emulatorDir/emulator.hpp
	g++ emulatorDir/mainEmulator.cpp emulatorDir/emulator.cpp -o emulatorDir/emulator

clean:
	rm -rf assemblerDir/lexer.c assemblerDir/lexer.h assemblerDir/parser.c assemblerDir/parser.h assemblerDir/assembler linkerDir/linker emulatorDir/emulator tests/*/*.hex tests/*/*.o