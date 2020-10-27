all: *.c
	gcc shell.c -o mysh -pedantic
zip: *.c makefile
	zip lab3 shell.c makefile
