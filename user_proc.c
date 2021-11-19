// Author: Lexi Anderson
// Last modified: Nov 18, 2021
// CS 4760, Project 5
// user_proc.c


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "clock.h"
#include "queue.h"
#include "shm.h"


static char* executable;

static int shmid = -1;
static int msgqid = -1;

static SysData* sysdata = NULL;
static Message msg;

int pidsim;

// function declarations
void initIPC();
void errexit(char*);
void sighandler(int);



int main(int argc, char** argv) {
	// signal handlers
	signal(SIGABRT, sighandler);
	signal(SIGUSR1, sighandler);

	executable = argv[0];  // get current executable name
	pidsim = atoi(argv[1]);

	initIPC();
	puts("User IPC initialized");

	// seed rand with a function of time, pid and bitwise ops
	srand((getpid() >> 8) ^ time(NULL));

	// variables
	Clock starttime, endtime, checktime;
	bool canterminate = false;  // has the process met termination criteria?
	bool willterminate = false;  // should the process terminate?
	bool allocatedrss = false;  // is the process currently holding resources?

	starttime = copyclock(sysdata->clock);
	checktime = copyclock(starttime);
	printf("Process %d starting at %d.%d\n", pidsim, sysdata->clock.s, sysdata->clock.ns);

	while (!willterminate) {
		// wait to receive message
		msgrcv(msgqid, &msg, sizeof(Message), getpid(), 0);

		printf("Process p%d received message\n", pidsim);

		// update termination eligibility
		if (!canterminate) {
			// update end time to current system time
			endtime = copyclock(sysdata->clock);

			// check if 1 sec has elapsed since start
			if (getclockdiff(endtime, starttime) >= MAX_NS)
				canterminate = true;
		}

		// choose process activity: request, release, or terminate
		int act;
		// loop until valid activity
		do {
			act = rand() % 3;
		} while ((act == 1 && !allocatedrss) || (act == 2 && !canterminate));

		printf("Activity %d chosen for process p%d\n", act, pidsim);
		msg.type = 1;
		msg.activity = act;

		switch (act) {
			case 0: // request
				for (int i = 0; i < NUM_RSS; i++) {
					// get max request amount
					int max = sysdata->pcb[pidsim].maximum[i] - sysdata->pcb[pidsim].allocation[i];
					msg.request[i] = rand() % (max + 1);
				}

				msgsnd(msgqid, &msg, sizeof(Message), 0);
				msgrcv(msgqid, &msg, sizeof(Message), getpid(), 0);

				if (msg.gotrss) allocatedrss = true;
				msg.gotrss = false;  // reset
				break;

			case 1: // release
				msgsnd(msgqid, &msg, sizeof(Message), 0);
				break;

			case 2: // terminate
				msgsnd(msgqid, &msg, sizeof(Message), 0);
				exit(pidsim);
		}

		// check termination criteria every 0-250ms

		int ns = (rand() % 250) * 1e6;  // 1ms = 10^6ns
		checktime = addtoclock(checktime, ns);  // update check time

		while (getns(checktime) > getns(sysdata->clock));  // wait before checking again
	}

	return(pidsim);
}

// Set up shared memory, message queue
void initIPC() {
	// shared memory
	key_t key;
	if ((key = ftok(IPC_FTOK, 0)) == -1) errexit("ftok");
	if ((shmid = shmget(key, sizeof(SysData), 0)) == -1)
		errexit("shmget");
	if ((sysdata = (SysData*)shmat(shmid, NULL, 0)) == (void*)(-1))
		errexit("shmat");

	// message queue
	if ((key = ftok(IPC_FTOK, 2)) == -1) errexit("ftok");
	if ((msgqid = msgget(key, 0)) == -1)
		errexit("msgget");
}

// 
void errexit(char* msg) {
	char errmsg[1024];
	snprintf(errmsg, 1024, "%s: %s", executable, msg);
	perror(errmsg);

	exit(1);
}

void sighandler(int signum) {
	printf("Terminating process %d\n", getpid());
	exit(1);
}
