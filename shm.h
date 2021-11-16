// Lexi Anderson
// Last modified: Nov 16, 2021
// CS 4760, Project 5
// shm.h -- Implementation of shared memory and system data structures

#ifndef SHM_H
#define SHM_H

#include <stdlib.h>
#include "clock.h"

#define MAX_USER_PROCS 18
#define NUM_RSS 20  // number of different resource types

#define MSG_SZ 1024


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

typedef struct {
	Clock clock;
	PCB pcb[MAX_USER_PROCS];

} SysData;

typedef struct {
	char str[MSG_SZ];
	int type;
} Message;


// function declarations
void initresources();

#endif
