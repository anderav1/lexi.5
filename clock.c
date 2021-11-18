// Lexi Anderson
// Last modified: Nov 10, 2021
// CS 4760, Project 5
// clock.c -- Define Clock functionality


#include <stdlib.h>
#include <string.h>
#include "clock.h"


Clock setclock(Clock clock, int ns) {
	clock.s = 0;
	clock.ns = ns;

	while (clock.ns >= MAX_NS) {  // can be converted to s
		clock.s += 1;
		clock.ns -= MAX_NS;
	}
	return clock;
}

// Get the clock value in ns
int getns(Clock clock) {
	return ((clock.s * MAX_NS) + clock.ns);
}

// Add ns to clock
Clock addtoclock(Clock clock, int ns) {
	clock.ns += ns;
	while (clock.ns >= MAX_NS) {
		clock.s += 1;
		clock.ns -= MAX_NS;
	}
	return clock;
}

// Rewind Clock tar by the value of Clock diff
Clock subtractfromclock(Clock tar, Clock diff) {
	// convert to ns
	int tartons = getns(tar);
	int difftons = getns(diff);

	// set clock
	return setclock(tar, tartons - difftons);
}

// Reset clock to zero
Clock resetclock(Clock clock) {
	clock.s = 0;
	clock.ns = 0;

	return clock;
}

// Copy the value of Clock src to Clock tar
Clock copyclock(Clock tar, Clock src) {
	tar.s = src.s;
	tar.ns = src.ns;

	return tar;
}

// Get minuend - subtrahend in ns
int getclockdiff(Clock minuend, Clock subtrahend) {
	Clock temp;
	copyclock(temp, minuend);
	subtractfromclock(temp, subtrahend);
	return getns(temp);
}
