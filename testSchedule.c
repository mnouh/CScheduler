#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <linux/unistd.h>
#define __NR_sys_start_pid_record 300
#define __NR_sys_stop_pid_record 301
#define MAX_CHAR 100
#define MAX_TIME 100

typedef struct{
	unsigned long long jif;
	pid_t threadGroupID;
	pid_t processID;
}ScheduleData;

int main(int argc, char* argv[]){
	int numProcesses = atoi(argv[1]);
	int numThreads = atoi(argv[2]);
	int interval = atoi(argv[3]);
	pid_t pid;
	int i;

	syscall(__NR_sys_start_pid_record);

	for (i = 0; i < numProcesses; i++){	
		pid = fork();
		
		if(pid < 0){
			printf("(%d)Fork failed to execute\n", getpid());
			exit(-1); 
		}else if(pid == 0){
			char numThreadCharArray[MAX_CHAR];
			char intervalArray[MAX_TIME];
			sprintf(numThreadCharArray, "%d", numThreads);
			sprintf(intervalArray, "%d", interval);	
			if(execlp("./mythreads", "./mythreads", numThreadCharArray, intervalArray, (char*) NULL) == -1){
				printf("\n\texeclp failed\n");
				exit(-1);
			}
		}
	}
	
	for(i = 0; i < numProcesses; i++){			
		wait(NULL);
	}
	
	ScheduleData *myScheduleData = malloc(sizeof(ScheduleData[1000000]));	
	if (syscall(__NR_sys_stop_pid_record, myScheduleData) == 1){
		printf("ERROR: stop_pid_record system call");
		exit(-1);
	}

	for(i = 0; myScheduleData[i].jif != 0; i++){
		printf("PID: %llu, TID: %llu, Jiffes: %llu\n", (unsigned long long) myScheduleData[i].processID, (unsigned long long) myScheduleData[i].threadGroupID,  myScheduleData[i].jif);
	}
	free(myScheduleData);

	return 0;
} 	
