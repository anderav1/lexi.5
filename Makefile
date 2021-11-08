# Author: Lexi Anderson
# Last modified: Nov 8, 2021
# CS 4760, Project 5
# Makefile

CC = gcc
CFLAGS = -g -Wall -Wshadow
TAR = oss user_proc
DEPS = oss.c user_proc.c
OBJ = oss.o user_proc.o


all: oss user_proc

$(TAR): %: %.o
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ): %.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -o $@ -c $<


# cleanup
.PHONY: clean
clean:
	rm -f $(TAR) $(OBJ)
