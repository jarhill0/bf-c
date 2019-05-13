CC = gcc
FLAGS = -g -pedantic -std=c99 -Wall -Wextra -Werror

interpreter: interpreter.o abstract_file.o
	$(CC) $(FLAGS) -o interpreter interpreter.o abstract_file.o

interpreter.o: interpreter.c abstract_file.h
	$(CC) $(FLAGS) -c interpreter.c

abstract_file.o: abstract_file.c abstract_file.h
	$(CC) $(FLAGS) -c abstract_file.c

clean:
	rm -f interpreter *.o
	rm -rf *.dSYM
