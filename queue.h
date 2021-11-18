// Lexi Anderson
// Last modified: Nov 17, 2021
// CS 4760, Project 5
// queue.h


#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>


typedef struct {
	int head, tail;  // array indices of first and last items
	int size, capacity;
	int* arr;
} Queue;


// function declarations

Queue* createqueue(int);
void pushq(Queue*, int);
int popq(Queue*);
int peekq(Queue*);
int getheadindex(Queue*);
bool queuefull(Queue*);
bool queueempty(Queue*);
void rotatequeue(Queue*);
void removefromqueue(Queue*, int);

#endif
