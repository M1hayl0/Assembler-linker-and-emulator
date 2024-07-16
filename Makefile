all:  lexer.l parser.y
	bison -d parser.y
	flex lexer.l
	cc -o $@ parser.tab.c lex.yy.c -lfl

clean:
	rm lex.yy.c parser.tab.c parser.tab.h all

run:
	./all < main.s