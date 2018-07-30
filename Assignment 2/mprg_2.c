/*
 ============================================================================
 Author      	: Menka J Mehta [12195032]
 Name		 	: Prg_2
 Project	 	: Assignment 3
 Subject	 	: Real-Time Operating Systems
 Compiler    	: GNU Compiler Collection (GCC) C
 Compiling CMD	: gcc -o Prg_2 Prg_2.c -lpthread -lrt
 				: ./Prg_2 
 Copyright   	: © 2018 Menka J Mehta. All rights reserved.
 Description 	: 
 ============================================================================
*/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define DATAFILE "Topic2_Prg_2.txt"
#define OUTPUTFILE "output_topic2.txt"

#define NUM_PROCESS 9
#define NUM_RESOURCE 3

int request[NUM_PROCESS][NUM_RESOURCE];
int allocation[NUM_PROCESS][NUM_RESOURCE];
int available[NUM_RESOURCE];

bool finish[NUM_PROCESS];
//complete_process stores successfully completed processes
int completed_process[NUM_PROCESS];
//incomplete_process stores incompleted processes
int incompleted_process[NUM_PROCESS];

bool complete = false;
int complete_count = 0;
int sequence_process[NUM_PROCESS];

//declare functions and procedures
void detector();
void readFromFile();
void requestCheck(int initial_resource, int initial_process);
int instructions(void);
void my_handler(int signum);

/**
 * This function provides details about the program 
 */
int instructions(void)
{
    printf("\n"
           "+----------------------------------------------------------------------------+\n"
           "PROGRAM AUTHOR"
           "\n+----------------------------------------------------------------------------+\n"
           "Real-Time Operating System: Assignment 3 Prg_2\n"
           "Menka Jagdish Mehta: 12195032\n"
           "22/05/2018"
           "\n+----------------------------------------------------------------------------+\n");
}

/**
 * Program sends user defines signals to the process
 */
void my_handler(int signum)
{
    if (signum == SIGUSR1)
    {
        printf("Received SIGUSR1!\n");
    }
}

int main(int argc, char *argv[])
{
	//write to output file
	FILE *output = fopen(OUTPUTFILE, "w");
	if (output == NULL)
	{
		perror("fopen output file error");
		if (fclose(output))
			perror("fclose output file error");
		exit(EXIT_FAILURE);
	}


    // Load program details and instructions yasin details
    instructions();
	//call the readFromFile to read data from Topic2_Prg_2.txt
	readFromFile();
	//function with algorithms to detect deadlocks
	detector();
	// No deadlock output sequence IDS
	if (complete_count == NUM_PROCESS)
	{
		// Print to console
		printf("No deadlock detected.\nProcess sequence IDs: ");
		//print to the output file 
		fprintf(output, "No deadlock detected.\nProcess sequence IDs: ");
		for (int i = 0; i < (NUM_PROCESS); i++)
		{
			// Print to console
			printf("P%i\t", sequence_process[i]); 
			// Print to the output file
			fprintf(output, "P%i\t", sequence_process[i]);
		}
		printf("\n");
	}

	
	if (complete_count < NUM_PROCESS)
	{
		printf("No deadlock detected.\nDeadlocked process IDs: ");
		fprintf(output, "No deadlock detected.\nProcess sequence IDs: ");
		for (int j = 0; j < (NUM_PROCESS); j++)
		{
			//printf("P%i\t", sequence_process[j]);
			if (!(finish[j]))
			{
				printf("P%d\t", j);
				fprintf(output, "P%i\t", j);
			}
		}
		printf("\n");
	}
	return 0;
}
// Read from file
void readFromFile()
{
	char line[256]; 
	FILE *proc_fp = fopen(DATAFILE, "r");
	
	if (proc_fp == NULL)
	{ //Error handling
		perror("Error: opening proc_fp");
		if (fclose(proc_fp))
			perror("Error: closing proc_fp");
		exit(EXIT_FAILURE);
	}

	//Scan the first 2 lines
	fgets(line, sizeof(line), proc_fp);
	puts(line);
	fgets(line, sizeof(line), proc_fp);
	puts(line);

	// Scan first process including available
	fscanf(proc_fp, "%s %i %i %i %i %i %i %i %i %i", line, &allocation[0][0], &allocation[0][1], &allocation[0][2], &request[0][0], &request[0][1], &request[0][2], &available[0], &available[1], &available[2]);
	// TEST
	printf("%s	%i %i %i	%i %i %i	%i %i %i\n", line, allocation[0][0], allocation[0][1], allocation[0][2], request[0][0], request[0][1], request[0][2], available[0], available[1], available[2]);

	// scan all the remaining processes
	for (int i = 1; i < NUM_PROCESS; i++)
	{
		fscanf(proc_fp, "%s %i %i %i %i %i %i", line, &allocation[i][0], &allocation[i][1], &allocation[i][2], &request[i][0], &request[i][1], &request[i][2]);
		//TEST
		printf("%s	%i %i %i	%i %i %i\n", line, allocation[i][0], allocation[i][1], allocation[i][2], request[i][0], request[i][1], request[i][2]);
	}

	fclose(proc_fp);
	//send user defines seignals to the process
	printf("Writing to output_topic2.txt has finished\n");

	//handle signal
	signal(SIGUSR1, my_handler);
    raise(SIGUSR1);
}

// Function deadlock detection
// Loop through till no-deadlock or till process being try count twice (being looped 2times continuously?)
void detector()
{

	/* Check for empty allocation resources in all processes */
	for (int i = 0; i < NUM_PROCESS; i++)
	{						
		 // Loop through processes and set varaible to keep track of each resource sum				   
		int sum_resource = 0;	
		// Loop through resources			  
		for (int j = 0; j < NUM_RESOURCE; j++) 
			sum_resource += allocation[i][j];
		if (sum_resource == 0)
		// Set finish to true if no resource in allocation
		// Set finish to false if resource available in allocation
			finish[i] = true; 
		else
			finish[i] = false; 
	}

	int initial_resource = NUM_RESOURCE;
	int initial_process = NUM_PROCESS;
	int incomplete_count = 0;
	// Variable to check the current process sequence
	int sequence = 0;	
	// bool for request < work			
	bool checkrequest[initial_process]; 

	//Set available resource to work
	int work[NUM_RESOURCE];
	for (int i = 0; i < initial_resource; i++)
		work[i] = available[i];

	// check if any finished is false and hasn't been loop through twice
	// or set a global incomplete flag and the same value has not been loop over twice continuously?
	while (complete == false)
	{
		// Check if request is less than work for all false finish */
		// Loop through each process
		for (int i = 0; i < initial_process; i++) 
		{
			// Counter to compare request and work
			int count = 0; 
			//Check if request is less than work for all false finish
			while (count < initial_resource && finish[i] == false) // Loop through each resource
			{
				// Comparing each resource
				if (request[i][count] <= work[count]) 
				{
					checkrequest[i] = true;
					// Increment count to compare next value
					count++; 
				}
				else
				{
					checkrequest[i] = false;
					// Reset count
					count = 0; 
					break;	 
				}
			} 

			// Process 3
			if (checkrequest[i] && finish[i] == false) //Check if request < work and finish is already false
			{
				//printf("work: "); // TEST
				for (int j = 0; j < initial_resource; j++)
				{
					work[j] += allocation[i][j];
					//printf("%d ",work[j]); //TEST
				}

				finish[i] = true;
				// Save completed process id
				sequence_process[sequence] = i; // Store everything in order
				sequence++;

				printf("Process completed successfully: P%d\n", i);
				complete_count++;
				if (complete_count == NUM_PROCESS)
					complete = true;
				if (incomplete_count > 1)
					incomplete_count--;
			}
			// Check for false finish and havent been looped check
			else if (!(checkrequest[i]) && finish[i] == false && incomplete_count <= 1)
			{
				incomplete_count++;
				//printf("Incompleted\n");
				complete = false;
			}
			// Check for looped check
			else if (incomplete_count > 1)
			{
				//printf("looped\n");
				incomplete_count--;
				complete = true;
			}

		}
	}

	printf("Total process count: %i\n", complete_count);
}
