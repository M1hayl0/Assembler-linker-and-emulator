all: build assemblerAll linkerAll emulatorAll

build:
	mkdir -p build

assemblerAll: src/mainAssembler.cpp src/argumentTrasfer.c build/parser.c build/lexer.c src/assembler.cpp inc/argumentTrasfer.h inc/assembler.hpp
	g++ src/mainAssembler.cpp src/argumentTrasfer.c build/parser.c build/lexer.c src/assembler.cpp -o build/assembler

build/lexer.c: misc/lexer.l inc/argumentTrasfer.h
	flex -o build/lexer.c misc/lexer.l

build/parser.c: misc/parser.y misc/lexer.l inc/argumentTrasfer.h
	bison -o build/parser.c misc/parser.y

linkerAll: src/mainLinker.cpp src/linker.cpp inc/linker.hpp
	g++ src/mainLinker.cpp src/linker.cpp -o build/linker

emulatorAll: src/mainEmulator.cpp src/emulator.cpp inc/emulator.hpp
	g++ -pthread src/mainEmulator.cpp src/emulator.cpp -o build/emulator

clean:
	rm -rf build tests/*/*.hex tests/*/*.o