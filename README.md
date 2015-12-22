***Author***: Benjamin Jones

***Class***: Compiler Theory

WCompiler
=========

Description
-----------

This is a compiler implemented in C for the programming language specified in "project.pdf,"
located in this directory. The scanner is implemented using a generic finite state machine
as implemented in "fsm.c," which generates tokens from the source file one at a time. The 
recursive descent parser ("parser.c") performs syntax analysis, type checking, code
generation and error reports.

Requirement(s):
  * maketools
  * gcc
  * glib-2.0
  * pkg-config

***Note***: The machine code generated is designed for 32bit machines. To compile on the x86_64
architecture, 32bit libraries are needed.

BUILDING
========

Simply run 'make' to generate the executable 'wcompiler.out'

RUNNING
=======

***Syntax***: ./wcompiler.out -f <source_file> [-d]
  * Option -d provides extensive debugging.

This will generate a file named 'temp.c' in the current directory. To build the generated C,
simply run 'make runtime'.

The final executable will be named prgrm.out

TEST PROGRAMS
=============

Programs to test the compiler have been written and are in the testPgms directory.
