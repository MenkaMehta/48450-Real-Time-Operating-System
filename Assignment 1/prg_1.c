#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>

#define LINE_SIZE 127
#define DATA_FILENAME "data.txt"
#define SRC_FILENAME "src.txt"
#define BUF_SIZE 1024

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct ThreadData
{
	char *tempData;					
	int fd[2];								
	int eof;								
	pthread_mutex_t eof_mutex, tp_mutex;	
	sem_t write, read, justify;	
	pthread_t tidA,tidB,tidC;			
};

struct timeval start, end;

/*Function Headers*/
void *threadA_subroutine(void *args);	    
void *threadB_subroutine(void *args);		
void *threadC_subroutine(void *args);		
void init(void *args);			
void terminate(void *args);				
int eof(struct ThreadData *threadData);		
void writeToSharedMemory(double *time); 
						

/*
 * Function main
 * executes the main program
 */
int main(int argc, char *argv[])
{
	//Start timer 
	//clock_t time_start = clock();
	
	gettimeofday(&start, NULL);

	struct ThreadData threadData;
	
    //initialise threadData
    init(&threadData);
    perror("thread initialised");
    
    //create thread attributes, set to default
	pthread_attr_t attr;
	pthread_attr_init(&attr);
    
    perror("thread attributes");
	   
	//create threads
	if (pthread_create(&threadData.tidA, &attr, threadA_subroutine, &threadData) == 0)
	    perror("Thread A Created.\n");
	else
	{
	    handle_error("pthread A create error");
	    terminate(&threadData);
	    exit(0);
	}
	    
	if(pthread_create(&threadData.tidB, &attr, threadB_subroutine, &threadData) == 0)
	    perror("Thread B created.\n");
	else
	{
	    handle_error("pthread B create error");
	    terminate(&threadData);
	    exit(0);   
	}

	if(pthread_create(&threadData.tidC, &attr, threadC_subroutine, &threadData) == 0)
	    perror("Thread C created.\n");
	else
	{
        handle_error("pthread C create error");
	    terminate(&threadData);
	    exit(0);  
	}
	
	perror("all working till here");
        
    //wait for threads to finish
	pthread_join(threadData.tidA,NULL);	
	pthread_join(threadData.tidB,NULL);       
	pthread_join(threadData.tidC,NULL);
	    
	perror("threads are working!");
   
	//terminate threads and semaphores and free resources
	terminate(&threadData);
	perror("threads terminated successfully");

	//NEW TEST
	//clock_t time_end = clock();
	gettimeofday(&end, NULL);
	//double time = (double)(time_end - time_start)/ CLOCKS_PER_SEC;
	//double elapsed = (end.tv_sec - begin.tv_sec)/1000 to millisec + 
              //((end.tv_usec - begin.tv_usec)/1000000.0))*1000 to millisec;
    //------double time = (end.tv_usec - start.tv_usec);
    double time = ((end.tv_sec - start.tv_sec)*1000 +(end.tv_usec - start.tv_usec)/1000); 
    //writing to shared memory
	writeToSharedMemory(&time);
	printf("The execution time is.....\nRuntime: %f millisecond(s)\n", time);
	 
    return 0;
}

/*
 * init -> initilises all the mutexes and semaphores
 *
 */
void init(void *args) {

	
	struct ThreadData *threadData = args;

	// memory allocation
	threadData->tempData = malloc(LINE_SIZE*sizeof(int));
	if (threadData->tempData == NULL)
	{
        handle_error("Error: tempData");
        exit(1);
	}

    //mutex initialisation
	pthread_mutex_init(&(threadData->eof_mutex), NULL); 
	pthread_mutex_init(&(threadData->tp_mutex), NULL); 

	//set eof flag to 0
	pthread_mutex_lock(&(threadData->eof_mutex));
	threadData->eof = 0;
	pthread_mutex_unlock(&(threadData->eof_mutex));

    //semaphore initiliasation
	sem_init(&(threadData->write), 0, 0);   
	sem_init(&(threadData->read), 0, 0);	
	sem_init(&(threadData->justify), 0, 0);
	sem_post(&(threadData->write));			

	//generate pipe
	if(pipe(threadData->fd)<0)
    {
        handle_error("Error: pipe error");
	}
	
	//free malloc
	free(threadData->tempData);
}

/*
 * Function threadA
 * Thread A reads 1 line of data from data.txt file and writes it to the pipe
 */
void *threadA_subroutine(void *args)
{
	//variable declaration for thread A
	struct ThreadData *threadData = args;
	FILE *source;

	//Open data.txt file 
	if((source = fopen(DATA_FILENAME, "r")))
	{
		char line[LINE_SIZE];

		//while end of file not reached, loop
		while(fgets(line,sizeof line, source) != NULL)
		{
		    //wait for semaphore
			sem_wait(&(threadData->write));	
			//then write line to the pipe				
			write(threadData->fd[1], line, strlen(line));
			//signal thread B 	
			sem_post(&(threadData->read));					
		}

		// Close data.txt file 
		fclose(source);
		perror("trial thread A.....");
		
        //mutex lock
		//pthread_mutex_lock(&(threadData->eof_mutex));
		//set eof to 1, notify other threads that data.txt file has finished
		threadData->eof = 1;
		//mutex unlock								
		//pthread_mutex_unlock(&(threadData->eof_mutex));	

	}else
	{
		
		handle_error("Error: data.txt"); 
		//mutex lock                         
		pthread_mutex_lock(&(threadData->eof_mutex));
		// Set eof to 2, so other threads don't wait for data	
		threadData->eof = 2;	
		//mutex unlock						
		pthread_mutex_unlock(&(threadData->eof_mutex));	
		
		terminate(&threadData);
		exit(0);
		
	}
    //signal thread B
	sem_post(&(threadData->read));
	sem_wait(&(threadData->write));
	
	pthread_cancel(threadData->tidA);
	pthread_cancel(threadData->tidB);
	pthread_cancel(threadData->tidC);
	
    perror("thread A completed....");
	pthread_exit(NULL);

}

/*
 * Function threadB
 * Reads temporary data from the pipe and passes it to thread C
 */
void *threadB_subroutine(void *args)
{
// variable declaration for thread B
	struct ThreadData *threadData = args;
	int n;

	//while eof not reached, keep looping
	while(eof(threadData) == 0)
	{
		//Wait for semaphore
	  	sem_wait(&(threadData->read));

		//if datafile opening in thread A failed,
		if(eof(threadData) == 2)
		{
			sem_post(&(threadData->justify));	
			// exit thread
			pthread_exit(NULL);					
		}

		// read data from pipe and pass to thread C
		pthread_mutex_lock(&(threadData->tp_mutex));
		n = read(threadData->fd[0], threadData->tempData, LINE_SIZE);
		threadData->tempData[n] = '\0'; 			
		pthread_mutex_unlock(&(threadData->tp_mutex));

	
		if(eof(threadData) == 1)
		{
		    // if eof reached, signal thread C
			sem_post(&(threadData->justify));	
			// exit thread
			pthread_exit(NULL);					
		}
		
		//signal thread C
	    sem_post(&(threadData->justify));
	}
	//clear read pipe
	close(threadData->fd[0]);
    //Signal threadC one last time for thread completion
    sem_post(&(threadData->justify));
    
    perror("thread B completed");
    
	pthread_exit(NULL);
}

/*
 *Thread C reads line of characters from B and detects whether the characters
 * are from the file header region or content region. Ignores data in the 
 * header region and write data from the content region to src.txt file.
 */
void *threadC_subroutine(void *args)
{
	//variable declaration for thread C
	struct ThreadData *threadData = args;
	FILE *source; 
	int end_header = 0;

	//open src.txt file
	source = fopen(SRC_FILENAME, "w");

	//while eof not reached, keep looping
	while(eof(threadData) == 0)
	{
		//wait for semaphore
		sem_wait(&(threadData->justify));

		//if datafile opening in thread A failed,
		if(eof(threadData) == 2)
		{
		    //close source file
			fclose(source);	
			//exit thread		
			pthread_exit(NULL);		
		}

		//get data from thread B, and if end header is detected,
		if(end_header)	
		    //write data to the src.txt						
			fputs(threadData->tempData,source);	
			
			//handle_error(threadData->tempData);
			
		else if (strstr(threadData->tempData,"end_header")) // Look for end header
			end_header = 1;								

		if(eof(threadData) == 1)
		{
		    //close source file
			fclose(source);	
			//exit thread		
			pthread_exit(NULL);	
		}

		// signal thread A
	  	sem_post(&(threadData->write));
	}

    //close source src.txt file
	fclose(source);
	
	// signal thread A to finish up tasks, closing all threads
	sem_post(&(threadData->write));
	
    perror("Thread C completed...");
	
	//return 0;
	pthread_exit(NULL);
}

/*
 * Function terminate
 * terminates/destroy mutexes and semaphores and resets allocated data
 */
void terminate(void *args)
{

	struct ThreadData *threadData = args;
	
    close(threadData->fd[0]);
    close(threadData->fd[1]);
	free(threadData->tempData);	
	//destroy semaphores
	sem_destroy(&(threadData->write));					
	sem_destroy(&(threadData->read));					
	sem_destroy(&(threadData->justify));		
	//destroy mutexes			
	pthread_mutex_destroy(&threadData->eof_mutex);	
	pthread_mutex_destroy(&threadData->tp_mutex);	
				
}

/*
 * Function eof
 * helps while accessing the eof flag and while using mutexes
 * returns: int 0 = not end of file, 1 = end of file,
 * 2 = error opening file, -1 = unknown error
 */
int eof(struct ThreadData *threadData)
{
	
	pthread_mutex_lock(&(threadData->eof_mutex));
	if(threadData->eof == 0)
	{
		pthread_mutex_unlock(&(threadData->eof_mutex));
		return 0;
	}else if (threadData->eof == 1)
	{
		pthread_mutex_unlock(&(threadData->eof_mutex));
		return 1;
	}else if(threadData->eof == 2)
	{
		pthread_mutex_unlock(&(threadData->eof_mutex));
		return 2;
	}else
	{
		pthread_mutex_unlock(&(threadData->eof_mutex));
		return -1;
	}
}

/*
 * writeToSharedMemory
 * writes data to shared memory as instructed in the assignment
 * 
 */
void writeToSharedMemory(double *time)
{
	int shared_mem, retval;		
	void *mem_address = NULL; 
	double *var;
	
	//shared memory init
	shared_mem = shmget((key_t)123456, 6, IPC_CREAT|0666);
	if(shared_mem < 0)
	{
	    handle_error("Error: memory creation failed\n");
	    shared_mem = shmget((key_t)123456, 6, 0666);   
	}
	
	//attach shared memory id to memory space
	mem_address = shmat(shared_mem, NULL, 0);
	if (mem_address == NULL)
	   handle_error("Error: Memory attachment failure\n");
	
	var = (double *)mem_address;
	memset(var, '\0', sizeof(*time));
	memcpy(var, time, sizeof(*time));
	
	retval = shmdt(var);
	if(retval < 0)
	    handle_error("Error: memory partition failed");
	    
}
	    
	
	

