// Author: Lexi Anderson
// Last modified: Nov 17, 2021
// CS 4760, Project 5
// oss.c


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <unistd.h>
#include "queue.h"
#include "shm.h"

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

void setupsighandler();
void sighandler(int);
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

void resetlogfile();
void log(char*, ...);
void printresources();
void printarray(char*, int[NUM_RSS]);
void printmatrix(char*, int[][NUM_RSS], Queue*, int);
void printstats();



int main(int argc, char** argv) {
	setupsighandler();
	resetlogfile();

	// TODO: check command line args

	executable = argv[0];

	// seed rand
	srand((getpid() << 16) ^ time(NULL));

	initsysdata();  // system data
	initIPC();  // shared mem, semaphore, msg queue
	initresources();  // resource descriptors
	memset(pids, 0, sizeof(pids));  // initialize all pids to 0

	// set next fork time
	nextForkTime.s = 0;
	nextForkTime.ns = 0;

	procq = createqueue(MAX_USER_PROCS);  // create empty process queue

	runsim();

// TODO: anything that needs to be done after simulation ends

	releaseIPC();
	puts("Program finished successfully");
	return(0);
}

void setupsighandler() {
	alarm(MAX_TIME); // SIGALRM

	signal(SIGINT, sighandler);
	signal(SIGALRM, sighandler);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGABRT, sighandler);
}

void sighandler(int signum) {
	// TODO: implement
	

	switch (signum) {
		case SIGALRM:
			// TODO: any logging?
			got_interrupt = 1;
			break;

		default:
			printf("Got signal %d\n", signum);
			printstats();

			// kill user procs
			kill(0, SIGUSR1);

			releaseIPC();
			exit(0);
	}

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

// Clean up shared memory, semaphore, message queue
void releaseIPC() {
	// shared memory
	if (sysdata != NULL && shmdt(sysdata) == -1) errexit("shmdt");
	if (shmid > 0 && shmctl(shmid, IPC_RMID, NULL) == -1) errexit("shmctl rmid");

	// semaphore
	if (semid > 0 && semctl(semid, 0, IPC_RMID) == -1) errexit("semctl rmid");

	// message queue
	if (msgqid > 0 && msgctl(msgqid, IPC_RMID, NULL) == -1) errexit("msgctl rmid");
}

// Lock semaphore
void locksem(int ind) {
	struct sembuf ops = { ind, -1, SEM_UNDO };
	if (semop(semid, &ops, 1) == -1) errexit("semop lock");
}

// Unlock semaphore
void unlocksem(int ind) {
	struct sembuf ops = { ind, 1, SEM_UNDO };
	if (semop(semid, &ops, 1) == -1) errexit("semop unlock");
}

// Run the simulation
void runsim() {
	// TODO: implement runsim function

	while (!got_interrupt) {
	
		updateclock();
		tryfork();
	}
}

// Send error message and exit
void errexit(char* msg) {
	char errmsg[MSG_SZ];
	snprintf(errmsg, MSG_SZ, "%s: %s", executable, msg);
	perror(errmsg);

	releaseIPC();
	exit(1);
}

// Check fork criteria
void tryfork() {
	if (activeprocs >= MAX_USER_PROCS) return;
	if (totalprocs >= MAX_PROCS_GENERATED) return;

	resetclock(nextForkTime);

	int pidsim = getpidsim();
	if (pidsim != -1) newuserproc(pidsim);
}

// Create a new user process
void newuserproc(int pidsim) {
	pid_t pid = fork();
	if ((pids[pidsim] = pid) == -1) errexit("fork");
	else if (pid == 0) { // child; generate new user proc
		printf("Executing user process %d\n", pidsim);
		char arg[MSG_SZ];
		snprintf(arg, MSG_SZ, "%d", pidsim);
		execl("./user_proc", "user_proc", arg, (char*)NULL);
		errexit("execl");
	}

	// parent
	setupPCB(pid, pidsim);
	pushq(q, pidsim);  // push new proc onto queue
	activeprocs++;
	totalprocs++;
}

// Get the index of the next available pid
int getpidsim() {
	for (int i = 0; i < MAX_USER_PROCS; i++) {
		if (pids[i] == 0) return i;
	} else return(-1);
}

// Set up PCB for a process
void setupPCB(pid_t pid, int pidsim) {
	PCB* pcb = &sysdata->pcb[pidsim];
	pcb->pid = pid;
	pcb->pidsim = pidsim;

	for (int i = 0; i < NUM_RSS; i++) {
		pcb->allocation[i] = 0;
		pcb->request[i] = 0;
		pcb->maximum[i] = (rand() % rss.instances[i]) + 1;
	}
}

// Increment the clock by a random interval
void updateclock() {
	locksem(0);  // lock clock
	int interval = rand() % (10 * MAX_NS) + 1;
	addtoclock(nextForkTime, interval);
	addtoclock(sysdata->clock, interval);
	unlocksem(0);  // unlock clock
}

// Erase any contents already in the log file
void resetlogfile() {
	FILE* fp;
	if ((fp = fopen(LOG_FILE, "w")) == NULL) errexit("fopen");
	if (fclose(fp) == EOF) errexit("fclose");
}

// Write to log
void log(char* format, ...) {
	FILE* fp;
	if ((fp = fopen(LOG_FILE, "a+")) == NULL) errexit("fopen");

	char msg[MSG_SZ];
	// create a variable arg list for any args
	va_list args;
	va_start(args, format);
	vsnprintf(msg, MSG_SZ, format, args);  // print va_list to string msg
	va_end(args);

	// print string to log
	fprintf(fp, msg);
/*TEST*/fprintf(stdout, msg);

	if (fclose(fp) == EOF) errexit("fclose");
}

// Print resource descriptors
void printresources() {
	if (queueempty()) errexit("queueempty");

	int maximum[qsize][NUM_RSS];  // maximum matrix
	int allocation[qsize][NUM_RSS];  // allocation matrix
	int available[NUM_RSS];  // availability vector

	// loop through resources
	for (int i = 0; i < NUM_RSS; i++)
		available[i] = rss.instances[i];

	// loop through queue
	for (int i = 0; i < q->size; i++) {
		while (q->arr[i] == 0) continue;  // skip empty

		for (j = 0; j < NUM_RSS; j++) :
			maximum[i][j] = sysdata->pcb[i].maximum[j];
			allocation[i][j] = sysdata->pcb[i].allocation[j];
			available[j] -= allocation[i][j];
		}
	}

	printarray("Total Resources", rss.instances);
	printarray("Shared Resources", rss.shareable);
	printarray("Available Resources", available);

	printmatrix("Allocated Resources", q, allocation);
	printmatrix("Maximum Resources", q, maximum);
}

// Print contents of array
void printarray(char* header, int arr[NUM_RSS]) {
	log("%s\n", header);

	for (int i = 0; i < NUM_RSS; i++) log(" R%2d", i);
	log("\n");

	for (int i = 0; i < NUM_RSS; i++) log(" %2d", arr[i]);
	log("\n");
}

// Print contents of matrix
void printmatrix(char* header, int mat[][NUM_RSS], Queue* q) {
	if (queueempty(q)) errexit("queueempty");

	log("%s\n", header);

	for (int i = 0; i < NUM_RSS; i++) log(" R%2d", i);
	log("\n");

	for (int i = 0; i < q->capacity; i++) {
		while (q->arr[i] == 0) continue;  // skip empty

		log("p%2d ", i);
		for (int j = 0; j < NUM_RSS; j++) {
			log(" %2d", mat[i][j]);
		}
		log("\n");
	}
}

// Print program statistics
void printstats() {
	log("\n\nProgram ended at system time %d.%d\n", sysdata->clock.s, sysdata->clock.ns);
	log("%d total processes executed\n", totalprocs);
	log("%d processes exited previously\n", exitedprocs);
	log("%d processes active at time of termination", activeprocs);
}
