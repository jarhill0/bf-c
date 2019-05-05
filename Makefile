CC = gcc
FLAGS = -g -pedantic -std=c99 -Wall -Wextra -Werror

interpreter: interpreter.c
	$(CC) $(FLAGS) -o interpreter interpreter.c

clean:
	rm -f interpreter
	rm -rf *.dSYM
