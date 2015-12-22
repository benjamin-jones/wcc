GLIB_INCLUDES=`pkg-config --cflags glib-2.0`
GLIB_LIBS=`pkg-config --libs glib-2.0`
default:
	make all
all:
	gcc -m32 -c fsm.c -o fsm.o
	gcc -m32 $(GLIB_INCLUDES) -c scanner.c -o scanner.o 
	gcc -m32 $(GLIB_INCLUDES) -c parser.c -o parser.o
	gcc -m32  parser.o fsm.o scanner.o -o wcompiler.out $(GLIB_LIBS)
runtime:
	gcc -m32 -c -m32 runtime.c -o runtime.o 
	gcc -m32 -c -m32 temp.c -o temp.o
	gcc -m32 runtime.o temp.o -o prgm.out -lm
clean:
	rm *.o
	rm *.out
