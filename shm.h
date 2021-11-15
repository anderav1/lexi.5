// Lexi Anderson
// Last modified: Nov 15, 2021
// CS 4760, Project 5
// shm.h -- Implementation of shared memory

#ifndef SHM_H
#define SHM_H

#include <stdlib.h>

#define MAX_USER_PROCS 18
#define NUM_RSS 20  // number of different resource types

// termination criteria
#define MAX_PROCS_GENERATED 40
#define MAX_TIME 5


// TODO: shared memory, semaphores, message queues

// allocate shared memory for system data structs

typedef struct {
	int instances[NUM_RSS];
	int shareable[NUM_RSS];

} Resource;

typedef struct {
	pid_t pid;
	int pidsim;  // simulated pid
	int allocation[NUM_RSS];  // allocated resources
	int request[NUM_RSS];  // requested resources
	int maximum[NUM_RSS];  // max number of instances of resource
} PCB;


#endif
