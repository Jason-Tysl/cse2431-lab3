all: *.c
	gcc shell.c -o mysh -pedantic
zip: *.c makefile README.md
	zip lab3 shell.c makefile README.md
