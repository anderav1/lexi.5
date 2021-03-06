// Author: Lexi Anderson
// Last modified: Nov 19, 2021
// CS 4760, Project 5
// oss.c


#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "clock.h"
#include "queue.h"
#include "shm.h"

// termination criteria
#define MAX_PROCS_GENERATED 40
#define MAX_TIME 5

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
void manageuserprocs();
bool deadlockdetect(Queue*, int, int[NUM_RSS]);
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
void printmatrix(char*, Queue*, int[][NUM_RSS]);
void printstats();


static char* executable = NULL;

static int shmid = -1;
static int semid = -1;
static int msgqid = -1;

static SysData* sysdata = NULL;

static Resource rss;  // resource descriptor
static Queue* q;
static Message msg;

static pid_t pids[MAX_USER_PROCS];
static int activeprocs = 0;
static int exitedprocs = 0;
static int totalprocs = 0;
static bool verbose = false;

volatile sig_atomic_t got_interrupt = 0;



int main(int argc, char** argv) {
	setupsighandler();
	resetlogfile();

	// TODO: check command line args

	executable = argv[0];

	// seed rand
	srand((getpid() << 16) ^ time(NULL));

	initIPC();  // shared mem, semaphore, msg queue
	initsysdata();  // system data
	memset(pids, 0, sizeof(pids));  // initialize all pids to 0
	q = createqueue(MAX_USER_PROCS);  // create empty process queue
	initresources();  // resource descriptors

	// set next fork time

/*TEST*/
/*
	Queue* example = createqueue(11);
	printqueue(example);

	for (int i = 0; i < 11; i++) {
		pushq(example, i);
		printqueue(example);
		if (i >= 4) {
			printf("Removing item %d\n", i - 4);
			removefromqueue(example, i - 4);
			printf("Head at array index %d\n", example->head);
			printqueue(example);
		}
	}
*/
/*ENDTEST*/

// TODO: queue is manifesting correctly; update oss to coordinate with changes
	runsim();

	printstats();
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
	switch (signum) {
		case SIGALRM:
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

	sysdata->nextForkTime.s = 0;
	sysdata->nextForkTime.ns = 0;

	// set initial values for pcb
	for (int i = 0; i < MAX_USER_PROCS; i++) {
		sysdata->pcb[i].pid = -1;
		sysdata->pcb[i].pidsim = -1;
	}
}

// Set up resource descriptors
void initresources() {
	for (int i = 0; i < NUM_RSS; i++) {
		// generate random number of instances
		rss.instances[i] = rand() % 10 + 1;  // int in [1, 10]
	}

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
	if ((shmid = shmget(key, sizeof(SysData), IPC_EXCL | IPC_CREAT | IPC_PERM)) == -1) {
		errexit("shmget");
	}
	if ((sysdata = (SysData*)shmat(shmid, NULL, 0)) == (void*)(-1)) {
		errexit("shmat");
	}

	// semaphore
	if ((key = ftok(IPC_FTOK, 1)) == -1) errexit("ftok");
	if ((semid = semget(key, 1, IPC_EXCL | IPC_CREAT | IPC_PERM)) == -1) {
		errexit("semget");
	}
	if (semctl(semid, 0, SETVAL, 1) == -1) errexit("semctl setval");

	// message queue
	if ((key = ftok(IPC_FTOK, 2)) == -1) errexit("ftok");
	if ((msgqid = msgget(key, IPC_EXCL | IPC_CREAT | IPC_PERM)) == -1) {
		errexit("msgget");
	}
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
	while (!got_interrupt && sysdata->clock.s < MAX_TIME) {
		tryfork();
		updateclock();
		manageuserprocs();
		updateclock();

		// wait for children to finish
		int status;
		pid_t pid = waitpid(-1, &status, WNOHANG);
		if (pid > 0) {
			int pidsim = WEXITSTATUS(status);  // terminated user process returns pidsim
			pids[pidsim] = 0;
			--activeprocs;
			++exitedprocs;
		}
	}

	if (got_interrupt && (exitedprocs == totalprocs)) {
		releaseIPC();
		return;
	}
	if (exitedprocs == MAX_PROCS_GENERATED) {
		releaseIPC();
		return;
	}
}

// Communicate with user procs via message queue and act accordingly
void manageuserprocs() {
	int next = q->head;  // index of next proc to run
	int count = 0;
	bool hasrss = false;
	int numrss = 0;

	while (!queueempty(q)) {
		updateclock();

		msg.pid = sysdata->pcb[next].pid;
		msg.pidsim = sysdata->pcb[next].pidsim;
		msg.type = msg.pid;

		// tell user proc to run
		msgsnd(msgqid, &msg, sizeof(Message), 0);

		// receive response from user proc
		msgrcv(msgqid, &msg, sizeof(Message), 1, 0);
/*TEST*/
		for (int i = 0; i < NUM_RSS; i++) {
			printf("%d, ", msg.request[i]);
		}

		// get user process activity
		switch (msg.activity) {
			case REQUEST:
				log("[%1d.%9d] %s detected process p%d requesting the following resources:\n", sysdata->clock.s, sysdata->clock.ns, executable, msg.pidsim);

				for (int i = 0; i < NUM_RSS; i++) {
					if (msg.request[i] == 0) continue;
					log("\t%d instances of R%d\n", msg.request[i], i);
				}
				log("\n");

				// check that request is safe
				bool safe = deadlockdetect(q, next, msg.request);
				if (safe) {
					hasrss = false;
					numrss = 0;
					for (int i = 0; i < NUM_RSS; i++) {
						if (msg.request[i] > 0) {
							hasrss = true;
							numrss++;
						}
						sysdata->pcb[next].allocation[i] += msg.request[i];
						msg.request[i] = 0;
					}

					msg.gotrss = (numrss > 0);
					log("[%2d.%9d] \t%s granted P%d request\n", sysdata->clock.s, sysdata->clock.ns, executable, next);
				} else {
					msg.gotrss = false;
					log("\t[%2d.%9d] %s denied P%d request\n", sysdata->clock.s, sysdata->clock.ns, executable, next);
				}

				msg.type = sysdata->pcb[next].pid;
				msgsnd(msgqid, &msg, sizeof(Message), 0);

				break;

			case RELEASE:
				log("[%2d.%9d] %s detected p%d releasing resources:\n", sysdata->clock.s, sysdata->clock.ns, executable, next);
				numrss = 0;

				for (int i = 0; i < NUM_RSS; i++) {
					if (sysdata->pcb[next].allocation[i] > 0) {
						hasrss = true;
						numrss++;
					}
				}

				if (numrss > 0) {
					for (int i = 0; i < NUM_RSS; i++) {
						int alloc = sysdata->pcb[next].allocation[i];
						if (alloc > 0) {
							log("\t%d instances of R%d\n", alloc, i);
						}
						sysdata->pcb[next].allocation[i] = 0;
					}
					log("\n");
				} else log("\tNo resources released.\n");

				break;

			case TERMINATE:
				log("[%2d.%9d] Process p%d terminated\n", sysdata->clock.s, sysdata->clock.ns, next);

				// release resources held by process
				log("Resources released:\n");
				hasrss = false;
				numrss = 0;
				for (int i = 0; i < NUM_RSS; i++) {
					if (sysdata->pcb[next].allocation[i] > 0) {
						hasrss = true;
						numrss++;
					}
				}

				if (numrss > 0) {
					for (int i = 0; i < NUM_RSS; i++) {
						int alloc = sysdata->pcb[next].allocation[i];
						if (alloc > 0) {
							log("\t%d instances of R%d\n", alloc, i);
						}
						sysdata->pcb[next].allocation[i] = 0;
						sysdata->pcb[next].maximum[i] = 0;
					}
					log("\n");
				} else log("\tNo resources released.\n");

				// remove process from queue
				removefromqueue(q, next);

				break;
		}

		printresources();
		if (msg.activity == TERMINATE) continue;
		count++;
		next = q->head;
	}
}

bool deadlockdetect(Queue* q, int ind, int request[NUM_RSS]) {
	int qsize = q->size;
	if (qsize == 0) return true;  // only 1 active process

	int next = 0;

//	int pidsim = q->head;
	int max[qsize][NUM_RSS];
	int alloc[qsize][NUM_RSS];
	int need[qsize][NUM_RSS];

	int req[NUM_RSS];
	int avail[NUM_RSS];  // actually available resources
	int dd[NUM_RSS];  // hypothetically available resources used in deadlock detection

	// copy user proc resource data
	for (int i = 0; i < qsize; i++) {
		if (q->arr[i] == -1) {
			memset(max[i], 0, qsize * sizeof(int));
			memset(alloc[i], 0, qsize * sizeof(int));
			memset(need[i], 0, qsize * sizeof(int));
			continue;
		}
		for (int j = 0; j < NUM_RSS; j++) {
			max[i][j] = sysdata->pcb[next].maximum[j];
			alloc[i][j] = sysdata->pcb[next].allocation[j];
			need[i][j] = max[i][j] - alloc[i][j];
		}
		next++;
	}

	for (int i = 0; i < NUM_RSS; i++) {
		avail[i] = rss.instances[i];
		req[i] = request[i];
	}

	for (int i = 0; i < qsize; i++) {
		for (int j = 0; j < NUM_RSS; j++)
			if (!rss.shareable[j]) avail[j] -= alloc[i][j];
		dd[i] = avail[i];
	}

	if (verbose) {
		printarray("Resource Requests", request);
		printmatrix("Resource Allocation", q, alloc);
		printmatrix("Maximum Resources", q, max);
		printmatrix("Resources Needed", q, need);
	}

	bool done[qsize];
	int allocseq[qsize];  // resource allocation sequence

	for (int j = 0; j < qsize; j++) {
		done[j] = false;
	}

	next = ind;
	for (int j = 0; j < NUM_RSS; j++) {
		if (need[next][j] < req[j]) {
			log("[%2d.%9d] %s detected that process p%d's request exceeds needed resources at time %d.%d\n", sysdata->clock.s, sysdata->clock.ns, executable, ind);
			if (verbose) {
				printarray("Available Resources", avail);
				printmatrix("Resources Needed", q, need);
			}
			return false;
		}

		if (req[j] <= avail[j]) {  // there are enough available rss
			avail[j] -= req[j];
			alloc[next][j] += req[j];
			need[next][j] -= req[j];
		} else {
			log("\t[%2d.%9d] Not enough available resources\n", sysdata->clock.s, sysdata->clock.ns);
			if (verbose) {
				printarray("Available Resources", avail);
				printmatrix("Resources Needed", q, need);
			}
			return false;
		}
	}

	// detect possible deadlocks
	int k = 0;
	while (k < qsize) {
		bool found = false;
		int j;
		for (int i = 0; i < qsize; i++) {
			if (!done[i]) {
				for (j = 0; j < NUM_RSS; j++)
					if (need[i][j] > dd[j]) break;

				if (j == NUM_RSS) {
					for (int n = 0; n < NUM_RSS; n++)
						dd[n] += alloc[i][n];

					allocseq[k++] = i;
					done[i] = true;
					found = true;
				}
			}
		}

		if (!found) {
			log("\tGranting request would result in unsafe state\n");
			return false;
		}
	}

	if (verbose) {
		printarray("Available Resources", avail);
		printmatrix("Resources Needed", q, need);
	}

	log("\tSafe state after granting request\n");
	return true;
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
	if (sysdata->clock.s >= MAX_TIME) return;
	if (q->size >= MAX_USER_PROCS) return;
	if (totalprocs >= MAX_PROCS_GENERATED) return;
	if (got_interrupt) return;
	if (sysdata->nextForkTime.s > sysdata->clock.s || 
		(sysdata->nextForkTime.s == sysdata->clock.s && sysdata->nextForkTime.ns > sysdata->clock.ns)) return;

	locksem(0);
	sysdata->nextForkTime.s = sysdata->clock.s;
	sysdata->nextForkTime.ns = sysdata->clock.ns;
	int ns = rand() % (int)1e6 + 1;
	sysdata->nextForkTime = addtoclock(sysdata->nextForkTime, ns);
	unlocksem(0);

	int pidsim = getpidsim();
	if (pidsim != -1) newuserproc(pidsim);
}

// Create a new user process
void newuserproc(int pidsim) {
	pid_t pid = fork();
	pids[pidsim] = pid;
	if (pid == -1) errexit("fork");
	else if (pid == 0) { // child; generate new user proc
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
	}
	return(-1);
}

// Set up PCB for a process
void setupPCB(pid_t pid, int pidsim) {
	PCB* pcb = &sysdata->pcb[pidsim];
	pcb->pid = pid;
	pcb->pidsim = pidsim;

	for (int i = 0; i < NUM_RSS; i++) {
		pcb->maximum[i] = (rand() % rss.instances[i]) + 1;
		pcb->allocation[i] = 0;
//		pcb->request[i] = 0;
	}
}

// Increment the clock by a random interval
void updateclock() {
	locksem(0);  // lock clock
	int interval = rand() % (10 * MAX_NS) + 1;  // increment clock
	sysdata->clock = addtoclock(sysdata->clock, interval);
	sysdata->nextForkTime = addtoclock(sysdata->nextForkTime, interval);
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
	fprintf(fp, "%s", msg);
/*TEST*/fprintf(stdout, "%s", msg);

	if (fclose(fp) == EOF) errexit("fclose");
}

// Print resource descriptors
void printresources() {
	if (queueempty(q)) errexit("queueempty");
	int qsize = q->size;

	int maximum[qsize][NUM_RSS];  // maximum matrix
	int allocation[qsize][NUM_RSS];  // allocation matrix
	int available[NUM_RSS];  // availability vector

	// loop through resources
	for (int i = 0; i < NUM_RSS; i++)
		available[i] = rss.instances[i];

	// loop through queue
	for (int i = 0; i < q->capacity; i++) {
		if (q->arr[i] == -1) continue;  // skip empty

		for (int j = 0; j < NUM_RSS; j++) {
			maximum[i][j] = sysdata->pcb[i].maximum[j];
			allocation[i][j] = sysdata->pcb[i].allocation[j];
			available[j] -= allocation[i][j];
		}
	}

	printarray("Total Resources", rss.instances);
	printarray("Shared Resources", rss.shareable);
	printarray("Available Resources", available);
	log("\n");
	printmatrix("Allocated Resources", q, allocation);
	printmatrix("Maximum Resources", q, maximum);
	log("\n--------------------\n");
}

// Print contents of array
void printarray(char* header, int arr[NUM_RSS]) {
	log("\n%s\n", header);

	for (int i = 0; i < NUM_RSS; i++) log(" R%-2d", i);
	log("\n");

	for (int i = 0; i < NUM_RSS; i++) log(" %-3d", arr[i]);
	log("\n");
}

// Print contents of matrix
void printmatrix(char* header, Queue* q, int mat[][NUM_RSS]) {
	if (queueempty(q)) errexit("queueempty");

	log("\n%s\n\t", header);

	for (int i = 0; i < NUM_RSS; i++) log("R%-2d ", i);
	log("\n");

	for (int i = 0; i < q->capacity; i++) {
		if (q->arr[i] == -1) continue;  // skip empty

		log("p%-2d\t", i);
		for (int j = 0; j < NUM_RSS; j++) {
			log(" %-2d ", mat[i][j]);
		}
		log("\n");
	}
}

// Print program statistics
void printstats() {
	log("\n\nProgram ended at system time [%2d.%9d]\n", sysdata->clock.s, sysdata->clock.ns);
	log("%d total processes executed\n", totalprocs);
	log("%d processes exited previously\n", exitedprocs);
	log("%d processes active at time of termination\n", activeprocs);
}
