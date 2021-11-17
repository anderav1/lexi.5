// Author: Lexi Anderson
// Last modified: Nov 17, 2021
// CS 4760, Project 5
// user_proc.c


#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "clock.h"
#include "queue.h"
#include "shm.h"


enum activity {
	REQUEST,
	RELEASE,
	TERMINATE,
};

static char* executable;

static int shmid = -1;
static int msgqid = -1;

static SysData* sysdata = NULL;

int pidsim;
static Message msg;


// function declarations

void initIPC();
void releaseIPC();

void errexit(char*);



int main(int argc, char** argv) {
	// signal handlers
	signal(SIGABRT, sighandler);
	signal(SIGUSR1, sighandler);

	executable = argv[0];  // get current executable name
	pidsim = atoi(argv[1]);

	initIPC();

	// seed rand with a function of time, pid and bitwise ops
	srand((getpid() >> 8) ^ time(NULL));

	// variables
	Clock starttime, endtime, checktime;
	bool canterminate = false;  // has the process met termination criteria?
	bool willterminate = false;  // should the process terminate?
	bool allocatedrss = false;  // is the process currently holding resources?

	copyclock(starttime, sysdata->clock);
	copyclock(checktime, starttime);

	while (!willterminate) {
		// wait to receive message
		msgrcv(msgqid, &msg, sizeof(Message), getpid(), 0);

		// update termination eligibility
		if (!canterminate) {
			// update end time to current system time
			copyclock(endtime, sysdata->clock);

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
		addtoclock(checktime, ns);  // update check time

		while (getns(checktime) > getns(sysdata->clock));  // wait before checking again
	}

	return(pidsim);
}

// Set up shared memory, message queue
void initIPC() {
	// shared memory
	key_t key;
	if ((key = ftok(IPC_FTOK, 0)) == -1) errexit("ftok");
	if ((shmid = shmget(key, sizeof(SysData), IPC_EXCL | IPC_CREAT | IPC_PERM)) == -1)
		errexit("shmget");
	if ((sysdata = (SysData*)shmat(shmid, NULL, 0)) == (void*)(-1))
		errexit("shmat");

	// message queue
	if ((key = ftok(IPC_FTOK, 2)) == -1) errexit("ftok");
	if ((msgqid = msgget(key, IPC_EXCL | IPC_CREAT | IPC_PERM)) == -1)
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
