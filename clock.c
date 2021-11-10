// Lexi Anderson
// Last modified: Nov 10, 2021
// CS 4760, Project 5
// clock.c -- Define Clock functionality


#include <stdlib.h>
#include "clock.h"

#define MAX_NS 1e9  // value at which to convert ns to s


void setclock(Clock* clock, int ns) {
	clock->s = 0;
	clock->ns = ns;

	while (clock->ns >= MAX_NS) {  // can be converted to s
		clock->s += 1;
		clock->ns -= MAX_NS;
	}
}

void addtoclock(Clock* clock, int ns) {
	clock->ns += ns;
	while (clock->ns >= MAX_NS) {
		clock->s += 1;
		clock->ns -= MAX_NS;
	}
}

// Rewind Clock tar by the value of Clock diff
void subtractfromclock(Clock* tar, Clock* diff) {
	// convert to ns
	int tartons = (tar->s * MAX_NS) + tar->ns;
	int difftons = (diff->s * MAX_NS) + diff->ns;

	// set clock
	setclock(tar, tartons - difftons);
}

// Reset clock to zero
void resetclock(Clock* clock) {
	clock->s = 0;
	clock->ns = 0;
}

// Copy the value of Clock src to Clock tar
void copyclock(Clock* tar, Clock* src) {
	memcpy(src, tar, sizeof(Clock));
}
