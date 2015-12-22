default:
	make all

all:
	gcc -g -c fsm.c -o fsm.o
	gcc -g `pkg-config --cflags glib-2.0` -c scanner.c -o scanner.o 
	gcc -g `pkg-config --cflags glib-2.0` -c parser.c -o parser.o
	gcc `pkg-config --cflags --libs glib-2.0` parser.o fsm.o scanner.o -o wcompiler.out
runtime:
	gcc -g -c -m32 runtime.c -o runtime.o 
	gcc -g -c -m32 temp.c -o temp.o
	gcc -m32 runtime.o temp.o -o prgm.out -lm
clean:
	rm *.o
	rm *.out
