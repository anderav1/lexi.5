// Author: Lexi Anderson
// Last modified: Nov 16, 2021
// CS 4760, Project 5
// oss.c


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include "queue.h"
#include "shm.h"

#define IPC_FTOK "./Makefile"
#define IPC_PERM (S_IRUSR | S_IWUSR)

// termination criteria
#define MAX_PROCS_GENERATED 40
#define MAX_TIME 5

static char* executable = NULL;

static int shmid = -1;
static int semid = -1;
static int msgqid = -1;

static SysData* sysdata = NULL;

static Resource rss;  // resource descriptor
static Queue* procq;
static Clock* nextForkTime;

static pid_t pids[MAX_USER_PROCS];
static int activeprocs = 0;
static int exitedprocs = 0;
static int totalprocs = 0;

volatile sig_atomic_t got_interrupt = 0;


// function declarations
void initsysdata();
void initresources();
void initIPC();
void releaseIPC();
void locksem(int);
void unlocksem(int);

void runsim();
void errexit(char*);
void tryfork();
void newuserproc(int);
int getpidsim();
void setupPCB(pid_t, int);
void updateclock();


int main(int argc, char** argv) {
	// TODO: check command line args

	executable = argv[0];

	initsysdata();  // system data
	initIPC();  // shared mem, semaphore, msg queue
	initresources();  // resource descriptors
	memset(pids, 0, sizeof(pids));  // initialize all pids to 0

	// set next fork time
	nextForkTime.s = 0;
	nextForkTime.ns = 0;

	procq = createqueue(MAX_USER_PROCS);  // create empty process queue

// TODO: set up log file

// TODO: set up signal handling

	runsim();

// TODO: anything that needs to be done after simulation ends

	releaseIPC();
	puts("Program finished successfully");
	return(0);
}


void initsysdata() {
	// set system clock
	sysdata->clock.s = 0;
	sysdata->clock.ns = 0;

	// set initial values for pcb
	for (int i = 0; i < MAX_USER_PROCS; i++) {
		sysdata->pcb[i].pid = -1;
		sysdata->pcb[i].pidsim = -1;
	}
}

// Set up resource descriptors
void initresources() {
	for (int i = 0; i < NUM_RSS; i++)
		// generate random number of instances
		rss.instances[i] = rand() % 10 + 1;  // int in [1, 10]

	// determine which resources are shareable
	int min = NUM_RSS * 0.15;
	int max = NUM_RSS * 0.25;
	int sharedratio = rand() % (max - (max - min)) + min;
	for (int i = 0; i < sharedratio; i++) {
		bool set = false;
		while (!set) {  // loop until a new shared resource has been set
			int ind = rand() % NUM_RSS;
			if (!rss.shareable[ind]) {
				rss.shareable[ind] = true;
				set = true;
			}
		}
	}
}

// Initialize interprocess communication
void initIPC() {
	// shared memory
	key_t key;
	if ((key = ftok(IPC_FTOK, 0)) == -1) errexit("ftok");
	if ((shmid = shmget(key, sizeof(SysData), IPC_EXCL | IPC_CREAT | IPC_PERM)) == -1)
		errexit("shmget");
	if ((sysdata = (SysData*)shmat(shmid, NULL, 0)) == (void*)(-1))
		errexit("shmat");

	// semaphore
	if ((key = ftok(IPC_FTOK, 1)) == -1) errexit("ftok");
	if ((semid = semget(key, 1, IPC_EXCL | IPC_CREAT | IPC_PERM)) == -1)
		errexit("semget");
	if (semctl(semid, 0, SETVAL, 1) == -1) errexit("semctl setval");

	// message queue
	if ((key = ftok(IPC_FTOK, 2)) == -1) errexit("ftok");
	if ((msgqid = msgget(key, IPC_EXCL | IPC_CREAT | IPC_PERM)) == -1)
		errexit("msgget");
}

void releaseIPC() {
	// shared memory
	if (sysdata != NULL && shmdt(sysdata) == -1) errexit("shmdt");
	if (shmid > 0 && shmctl(shmid, IPC_RMID, NULL) == -1) errexit("shmctl rmid");

	// semaphore
	if (semid > 0 && semctl(semid, 0, IPC_RMID) == -1) errexit("semctl rmid");

	// message queue
	if (msgqid > 0 && msgctl(msgqid, IPC_RMID, NULL) == -1) errexit("msgctl rmid");
}

void locksem(int ind) {
	struct sembuf ops = { ind, -1, SEM_UNDO };
	if (semop(semid, &ops, 1) == -1) errexit("semop lock");
}

void unlocksem(int ind) {
	struct sembuf ops = { ind, 1, SEM_UNDO };
	if (semop(semid, &ops, 1) == -1) errexit("semop unlock");
}

void runsim() {
	// TODO: implement runsim function

	while (!got_interrupt) {
		// TODO: see if it is possible to fork a new proc

		// TODO: update the clock

		tryfork();
	}
}

void errexit(char* msg) {
	char errmsg[1024];
	snprintf(errmsg, 1024, "%s: %s", executable, msg);
	perror(errmsg);

	releaseIPC();
	exit(1);
}

void tryfork() {
	if (activeprocs >= MAX_USER_PROCS) return;
	if (totalprocs >= MAX_PROCS_GENERATED) return;

	resetclock(nextForkTime);

	int pidsim = getpidsim();
	// TODO: get simulated pid

	if (pidsim != -1) newuserproc(pidsim);
}

void newuserproc(int pidsim) {
	pid_t pid = fork();
	if ((pids[pidsim] = pid) == -1) errexit("fork");
	else if (pid == 0) { // child; generate new user proc
		printf("Executing user process %d\n", pidsim);
		char arg[1024];
		snprintf(arg, 1024, "%d", pidsim);
		execl("./user_proc", "user_proc", arg, (char*)NULL);
		errexit("execl");
	}
	// parent
	// TODO: set up pcb for new process
	setupPCB(pid, pidsim);
	// TODO: push process onto the queue
	activeprocs++;
	totalprocs++;
}

// Get the index of the next available pid
int getpidsim() {
	for (int i = 0; i < MAX_USER_PROCS; i++) {
		if (pids[i] == 0) return i;
	} else return(-1);
}

void setupPCB(pid_t pid, int pidsim) {
	// TODO: implement function
}

// Increment the clock by a random interval
void updateclock() {
	locksem(0);
	int interval = rand() % (10 * MAX_NS) + 1;
	addtoclock(nextForkTime, interval);
	addtoclock(sysdata->clock, interval);
	unlocksem(0);
}
