// Lexi Anderson
// Last modified: Nov 10, 2021
// CS 4760, Project 5
// clock.h

#ifndef CLOCK_H
#define CLOCK_H


#include <stdlib.h>


typedef struct {
	unsigned int s;  // seconds
	unsigned int ns;  // nanoseconds
} Clock;


void setclock(Clock*, int);
void addtoclock(Clock*, int);
void subtractfromclock(Clock*, Clock*);
void resetclock(Clock*);
void copyclock(Clock*, Clock*);


#endif
