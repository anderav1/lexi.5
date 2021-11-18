// Lexi Anderson
// Last modified: Nov 17, 2021
// CS 4760, Project 5
// clock.h

#ifndef CLOCK_H
#define CLOCK_H


#include <stdlib.h>


#define MAX_NS (int)1e9  // value at which to convert ns to s

typedef struct {
	unsigned int s;  // seconds
	unsigned int ns;  // nanoseconds
} Clock;


void setclock(Clock, int);
int getns(Clock);
void addtoclock(Clock, int);
void subtractfromclock(Clock, Clock);
void resetclock(Clock);
void copyclock(Clock, Clock);
int getclockdiff(Clock, Clock);


#endif
