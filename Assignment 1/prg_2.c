#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Defines */
#define MESSLENGTH 1024

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

/*
 * Function main
 * Reads from shared memory and writes to console.</summary>
 */
int main()
{
    int shared_mem, retval;		
	void *mem_address = NULL; //shared memoryS
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
	
	//read from memory
	var = (double *)mem_address;
    printf("The execution time is.....\nRuntime: %f second(s)\n", *var);
	
	//detach from memory
	retval = shmdt(var);
	if(retval < 0)
	    handle_error("memory partition failed");
	exit(EXIT_SUCCESS);
	    
}
