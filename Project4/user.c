#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>

#define barrier_init(x,z) syscall(359,x,z)
#define barrier_wait(x) syscall(360,x)
#define barrier_destroy(x) syscall(361,x)
#define MS_TO_NS(x) ((x)*1e6)

int sync_num;

/* Runs the desired number of Synchronization cycles*/

void *Threadfn(void *ptr){
int i,nus;

unsigned int *mybarrier = (unsigned int *) ptr;

for(i=0;i<sync_num;i++)
{
	nus=rand()%5;
	usleep(nus);
	if( barrier_wait(*mybarrier) < 0 )
		printf("Synchronization failed\n");

}

return 0;

}


/* Initializes the barrier and creates the threads */

int ChildProcess(unsigned int thread_count, pthread_t thread[], unsigned int *mybarrier,unsigned int timeout)
{
int i,ret;

pid_t pid;
pid=getpid();
srand(time(NULL));


*mybarrier=barrier_init(thread_count,timeout);

for(i=0;i<thread_count;i++){

	ret=pthread_create(&thread[i],NULL,Threadfn,mybarrier);
	if(ret < 0)
		{	
			printf("Failed to create thread %d for process %d (returned %d)\n",i,pid,ret);
			exit(1);
		}

	}
return 0;

}



/********************************MAIN BODY***************************/
 
int main()
{
int pid_c1,pid_c2,i,stat;
unsigned long tout;
pid_t pid_returned;

printf("enter the timeout value in ns : ");
scanf("%lu",&tout);

printf("enter no:of synchronisations :");
scanf("%d",&sync_num);

/* FORKING the FIRST CHILD */

if((pid_c1=fork()) == 0)
{
	pthread_t thread[25];
	unsigned int mybarrier1, mybarrier2;

	if( ChildProcess(20,&thread[0],&mybarrier1,tout) < 0 )
	{	printf("Child Process with 20 threads failed ");
		return -1;
	}

	if ( ChildProcess(5,&thread[20],&mybarrier2,tout) < 0 )
	{	printf("Child Process with 5 threads failed");
		return -1;
	}

	for(i=0;i<25;i++){

	pthread_join(thread[i],NULL);

	}

	if( barrier_destroy(mybarrier1) < 0 )
		printf("Failed to destroy barrier 1");
	if ( barrier_destroy(mybarrier2) < 0 )
		printf("Failed to destroy barrier 2");
	return 0;

}
	
else if(pid_c1 < 0)
{

printf("Failed to fork child process 1 ");
return -1;

}

/* FORKING THE SECOND CHILD */

if((pid_c2=fork()) == 0)
{

	pthread_t thread[25];
	unsigned int mybarrier1, mybarrier2;

	if( ChildProcess(20,&thread[0],&mybarrier1,tout) < 0 )
	{	printf("Child Process with 20 threads failed ");
		return -1;
	}

	if ( ChildProcess(5,&thread[20],&mybarrier2,tout) < 0 )
	{	printf("Child Process with 5 threads failed");
		return -1;
	}

	for(i=0;i<25;i++){

	pthread_join(thread[i],NULL);

	}

	if( barrier_destroy(mybarrier1) < 0 )
		printf("Failed to destroy barrier 1");
	if ( barrier_destroy(mybarrier2) < 0 )
		printf("Failed to destroy barrier 2");
	return 0;


}

else if(pid_c2 < 0)
{

printf("Failed to fork child process 2");
return -1;

}

/* MAIN THREAD CONTINUES HERE */

	/* WAIT-ing on CHILD PROCESSES */

	printf("Parent Process called now with pid %d \n", getpid());
	pid_returned = wait(&stat);
	printf("Process with pid %d pid is over\n",pid_returned);
	pid_returned=wait(&stat);
	printf("Process with pid %d pid is over\n", pid_returned);
	return 0;

}

