// Lexi Anderson
// Last modified: Nov 16, 2021
// CS 4760, Project 5
// queue.c -- Define queue functionality


#include <stdlib.h>
#include <string.h>
#include "queue.h"


Queue* createqueue(int capacity) {
	Queue* q = (Queue*)malloc(sizeof(Queue));
	q->capacity = capacity;
	q->size = 0;
	q->head = 0;
	q->tail = capacity - 1;
	q->arr = (int*)malloc(sizeof(int) * q->capacity);

	return q;
}

void pushq(Queue* q, int item) {
	if (!queuefull(q)) {
		q->tail = (q->tail + 1) % q->capacity;  // fill queue circularly
		q->arr[q->tail] = item;
		q->size += 1;
	}
	// TODO: handle case where queue is full
}

int popq(Queue* q) {
	if (!queueempty(q)) {
		int item = q->arr[q->head];  // get first item in queue
		q->arr[q->head] = 0;  // remove item from queue
		q->head = (q->head + 1) % q->capacity;  // update head
		q->size -= 1;

		return item;
	} else return(-1);
}

int peekq(Queue* q) {
	if (!queueempty(q)) return q->arr[q->head];
	else return(-1);
}

bool queuefull(Queue* q) { return (q->size == q->capacity); }

bool queueempty(Queue* q) { return (q->size == 0); }

void printqueue(Queue* q) {
	// TODO: implement printqueue function
}