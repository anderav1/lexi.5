// Lexi Anderson
// Last modified: Nov 15, 2021
// CS 4760, Project 5
// shm.c


#include <stdlib.h>
#include "shm.h"

static Resource resource;

// initialize resource descriptors
void initresources() {
	for (int i = 0; i < NUM_RSS; i++) {
		// generate number of instances between 1 and 10
		resource.instances[i] = rand() % 10 + 1;
	}

	// determine which resources are shareable
	int sharedmin = NUM_RSS * 0.15;
	int sharedmax = NUM_RSS * 0.25;
	int sharedratio = rand() % (sharedmax - (sharedmax - sharedmin)) + sharedmin;
	for (int i = 0; i < sharedratio; i++) {
		bool set = false;
		while (!set) {
			int ind = rand() % NUM_RSS;
			if (!resource.shareable[ind]) {
				resource.shareable[ind] = true;
				set = true;
			}
		}
	}
}
