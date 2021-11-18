// Lexi Anderson
// Last modified: Nov 17, 2021
// CS 4760, Project 5
// clock.h

#ifndef CLOCK_H
#define CLOCK_H


#include <stdlib.h>


#define MAX_NS (unsigned int)1e9  // value at which to convert ns to s

typedef struct {
	unsigned int s;  // seconds
	unsigned int ns;  // nanoseconds
} Clock;


Clock setclock(Clock, int);
int getns(Clock);
Clock addtoclock(Clock, int);
Clock subtractfromclock(Clock, Clock);
Clock resetclock(Clock);
Clock copyclock(Clock, Clock);
int getclockdiff(Clock, Clock);


#endif
