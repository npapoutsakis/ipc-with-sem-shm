CC = gcc
CFLAGS = -Wall -g

all: parent

parent: parent_proc.c
	gcc -o parent parent_proc.c
	./parent

clean:
	rm -f parent
	clear
