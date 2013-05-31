#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <linux/unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <unistd.h> 

#define __NR_sys_start_pid_record 300
#define __NR_sys_stop_pid_record 301

typedef struct{
	unsigned long long jif;
	pid_t threadGroupID;
	pid_t processID;
}ScheduleData;

typedef struct{;
	sem_t pMutex;
	int run;
}SharedSem;

int num_threads; // number of threads
int test_interval; // seconds for which to run the experiment
int numProc; //number of processes to run
 // flag to control thread execution

SharedSem* mySem;

void *thread_func(void * arg) 
{
	
	int tid = (int)arg;
	int pid = getpid();
	int ppid = getppid();
	printf("Thread %d:%d created. \n", getpid(), tid);
	
	
	if (sem_wait(&mySem->pMutex) == -1){
		printf("sem_wait() failed: %s", strerror(errno));
		exit(-1);
	}

	if (sem_post(&mySem->pMutex) == -1){
		printf("sem_post() failed: %s", strerror(errno));
		exit(-1);
	}
	printf("Thread %d:%d ready. \n", getpid(), tid);
/*	while( !run ) 
		sleep(1);
*/
	/* This is the actual run of all threads */
	while( mySem->run ) {
		/* do nothing - just yield each time */ 

		// enabling this print may interfere with your thread scheduling
		//printf("Thread %d:%d yielding. \n", getpid(), tid); 

		// yield to another thread/process
		pthread_yield();
		//sleep(1);
	}

	printf("Thread %d:%d finished. \n", getpid(), tid);
}


void process(void)
{
	int i, ret;
	pthread_t *tid;

	if ((tid = malloc(num_threads*sizeof(pthread_t))) == NULL) {
		perror("malloc error:");
		exit(1);
	}
	
	struct shmid_ds buffer;
		
	key_t key;
	int shmFlag;
	int shmID;
	size_t size;

	if ((key = ftok("/proc", 'A')) == -1) {
		printf("Key creation failed: %s\n", strerror(errno));
		exit(1);
	}

	size = sizeof(*mySem);
	shmFlag = 0644 | IPC_CREAT;
	//Error check
	if ((shmID = shmget(key, size, shmFlag)) == -1) {
		printf("shmget: shmget failed: %s\n", strerror(errno)); 
		exit(-1); 
	}
	
	if ((mySem = shmat(shmID, NULL, shmFlag)) == (void *) -1) {
		printf("shmat: shmat failed: %s\n", strerror(errno));
		exit(-1);
	}
	
	if (sem_init(&mySem->pMutex, 1, 0) < 0){
		printf("sem_init failed on parent: %s\n", strerror(errno));
		exit(-1);
	}

	mySem->run = 1;

	pid_t procPid = getpid();
	// CALL YOUR SYSCALL TO START PROFILING HERE
	if (syscall(__NR_sys_start_pid_record) == -1){
		printf("ERROR: start_pid_record system call");
		exit(-1);
	}

	// create child threads and processes

	for(i = 0; i < numProc; i++){
		if (procPid > 0){
			if((procPid = fork()) < 0){
				printf("Fork failed to execute: %s", strerror(errno));
				exit(-1);
			}
		}	
	}
	
	if(procPid == 0){
		for(i=0; i<num_threads; i++) {
			
			ret = pthread_create(&tid[i], NULL, thread_func, (void *)i);
			if( ret ) {
				printf("pthread creation error ret=%d\n", ret);
				exit(-1);				
			}
		}
/*
		run = 1;

		sleep(test_interval);

		run = 0;
*/
		for(i=0; i<num_threads; i++) {
			ret = pthread_join(tid[i], NULL);
			if( ret ) {
				printf("pthread join error ret=%d\n", ret);
				// no need to exit - continue
			}	
		}
	}
	
	// CALL YOUR SYSCALL TO STOP PROFILING HERE
	if (procPid > 0){
		if (sem_post(&mySem->pMutex) == -1){
			printf("sem_post() failed: %s", strerror(errno));
			exit(-1);
		}

		sleep(test_interval);

		mySem->run = 0;	

		for(i=0; i<numProc; i++){
			wait(NULL);
		}

		ScheduleData *myScheduleData = malloc(sizeof(ScheduleData[1000000]));
		for (i = 0; i < 1000000; i++){
			myScheduleData[i].jif = 0;
		}
		if (myScheduleData == NULL){
			printf("ERROR: malloc failed");
			exit(-1);
		}	
		if (syscall(__NR_sys_stop_pid_record, myScheduleData) == -1){
			printf("ERROR: stop_pid_record system call");
			exit(-1);
		}
		for(i = 0; myScheduleData[i].jif > 0; i++){
			printf("PID: %d, TID: %d, Jiffes: %llu\n", myScheduleData[i].processID, myScheduleData[i].threadGroupID,  myScheduleData[i].jif);
		}

		free(myScheduleData);
		
		//Delete shared memory and sem
		if((key = shmctl(shmID, IPC_RMID, &buffer)) == -1){
			printf("shmctl failed: %s", strerror(errno));
			exit(-1);
		}
		if (sem_destroy(&mySem->pMutex) == -1){
			printf("sem_destroy failed: %s", strerror(errno));
			exit(-1);
		}
	}

		
	if((key = shmdt(mySem)) == -1){
		printf("shmdt failed: %s", strerror(errno));
		exit(2);
	}
	free(tid);
			
	return;
}

int main(int argc, char * argv[])
{

	if( argc < 4 ){
		printf("Usage: %s <num_threads> <test_interval> <num_processes>\n", argv[0]);
		exit(-1);
	}

	num_threads = atoi(argv[1]);
	test_interval = atoi(argv[2]);
	numProc = atoi(argv[3]);
	
	printf("num_threads %d test_interval %d\n", num_threads, test_interval);
	process();

	return 0;
}
