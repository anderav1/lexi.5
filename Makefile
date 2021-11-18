# Author: Lexi Anderson
# Last modified: Nov 16, 2021
# CS 4760, Project 5
# Makefile

CC = gcc
CFLAGS = -g -Wall #-Wshadow
TAR = oss user_proc

OSS = oss
OSS_C = oss.c
OSS_O = oss.o $(OBJ)

USER = user_proc
USER_C = user_proc.c
USER_O = user_proc.o $(OBJ)

DEPS = queue.h shm.h clock.h
OBJ = queue.o clock.o


# generate executables
all: $(OSS) $(USER)

$(OSS): $(OSS_O)
	$(CC) $(CFLAGS) -o $@ $^

$(USER): $(USER_O)
	$(CC) $(CFLAGS) -o $@ $^


# generate object files
$(OBJ): %.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -o $@ -c $<



# cleanup
.PHONY: clean
clean:
	rm -f $(TAR) $(TAR:=.o) $(OBJ)
