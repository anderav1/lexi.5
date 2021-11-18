// Lexi Anderson
// Last modified: Nov 18, 2021
// CS 4760, Project 5
// queue.c -- Define queue functionality


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"


Queue* createqueue(int capacity) {
	Queue* q = (Queue*)malloc(sizeof(Queue));
	q->capacity = capacity;
	q->size = 0;
	q->head = 0;
	q->tail = 0;
	q->arr = (int*)malloc(sizeof(int) * q->capacity);
	memset(q->arr, -1, sizeof(int) * q->capacity);

	return q;
}

void pushq(Queue* q, int item) {
	if (!queuefull(q)) {
		while (q->arr[q->tail] != -1) {
			q->tail = (q->tail + 1) % q->capacity;  // fill queue circularly
		}
		q->arr[q->tail] = item;
		q->size += 1;
	}
	rotatequeue(q);  // update queue rotation
	// TODO: handle case where queue is full
}

int peekq(Queue* q) {
	if (!queueempty(q)) return q->arr[q->head];
	else return(-1);
}

bool queuefull(Queue* q) { return (q->size == q->capacity); }

bool queueempty(Queue* q) { return (q->size == 0); }

// Rotate queue array so that head lies at index 0
// Realigns array indices with head value
void rotatequeue(Queue* q) {
	if (q->head == 0) return;

	int shift = q->head;
	int tmp[q->capacity];

	for (int i = 0; i < q->capacity; i++) {
		tmp[i] = q->arr[(shift + i) % q->capacity];
	}

	memcpy(q->arr, tmp, sizeof(int) * q->capacity);

	// update head and tail
	q->head = (q->head - shift) % q->capacity;
	q->tail = (q->tail - shift) % q->capacity;
}

// Remove item at index ind in underlying array
// Note: index denotes position in array, not offset from head
void removefromqueue(Queue* q, int ind) {
	if (queueempty(q)) return;

	// set value
	q->arr[ind] = -1;
	q->size -= 1;

	// update head and tail indices
	if (q->size == 0) {
		q->head = 0;
		q->tail = 0;
	} else if (ind == q->head) {
		while (q->arr[q->head] == -1)
			q->head = (q->head + 1) % q->capacity;  // update head
	} else if (ind == q->tail) {
		while (q->arr[q->tail] == -1)
			q->tail = (q->tail - 1) % q->capacity; // update tail
	}
	rotatequeue(q);  // realign queue so that head lies at array index 0
}

// Print contents of queue starting at head
void printqueue(Queue* q) {
	printf("Queue(%d): ", q->size);
	for (int i = 0; i < q->capacity; i++) {
		int j = (q->head + i) % q->capacity;
		if (q->arr[j] == -1) continue;
		printf("%d at i=%d, ", q->arr[j], j);
	}
/*	printf("\nArray: ");
	for (int i = 0; i < q->capacity; i++) {
		printf("%d ", q->arr[i]);
	}
*/
	printf("\n\n");
}
