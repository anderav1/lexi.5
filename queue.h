// Lexi Anderson
// Last modified: Nov 16, 2021
// CS 4760, Project 5
// queue.h


#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

typedef struct {
	int head, tail;
	int size, capacity;
	int* arr;
} Queue;


// function declarations

Queue* createqueue(int);
void pushq(Queue*, int);
int popq(Queue*);
int peekq(Queue*);
bool queuefull(Queue*);
bool queueempty(Queue*);
void printqueue(Queue*);


#endif
