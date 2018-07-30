/*
 ============================================================================
 Author      	: Menka J Mehta [12195032]
 Name		 	: Prg_1
 Project	 	: Assignment 3
 Subject	 	: Real-Time Operating Systems
 Compiler    	: GNU Compiler Collection (GCC) C
 Compiling CMD	: gcc -o Prg_1 Prg_1.c -lpthread -lrt
 				: ./Prg_1 
 Copyright   	: © 2018 Menka J Mehta. All rights reserved.
 Description 	: 
 ============================================================================
*/
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <semaphore.h>

#define MAXPROCESS 7           /* Number of processes */
#define MESSLENGTH 256         /* Read 256 bytes at a time */
#define FIFONAME "/tmp/myfifo" /* Temporary FIFO directory */

typedef struct
{
    int PID;
    int waitTime;
    int turnaroundTime;
    int remainingTIme;
    int arrivalTime;
    int remainingTime;
    int burstTime;
} struct_v;

typedef struct
{
    sem_t *sem_write;
    sem_t *sem_read;
} struct_thread1;

typedef struct
{
    sem_t *sem_read;
    sem_t *sem_write;
    FILE *fp;
} struct_thread2;

//Global variables
int n = 7;
int status = 0;
char string[256];
double totalWT;
double totalTT;

// Semaphore types
sem_t sem_read;  /* Semaphore for reading from FIFO */
sem_t sem_write; /* Semaphore for writing to FIFO */

//Thread ID
pthread_t thread1;
pthread_t thread2;

/**
 * @brief Program 1 instructions.
 * Provides details about the program.
 * @return void
 */
int instructions(void)
{
    printf("\n"
           "+----------------------------------------------------------------------------+\n"
           "PROGRAM AUTHOR"
           "\n+----------------------------------------------------------------------------+\n"
           "Real-Time Operating System: Assignment 3 Prg_1\n"
           "Menka Jagdish Mehta: 12195032\n"
           "22/05/2018"
           "\n+----------------------------------------------------------------------------+\n");
}

/**
 * This function initialises the Process IDs, arrivalTime and burstTime
 */
void init(struct_v *v, int n)
{
    int index;
    for (index = 0; index < n; index++)
    {
        v[0].PID = 1;
        v[1].PID = 2;
        v[2].PID = 3;
        v[3].PID = 4;
        v[4].PID = 5;
        v[5].PID = 6;
        v[6].PID = 7;

        v[0].arrivalTime = 8;
        v[1].arrivalTime = 10;
        v[2].arrivalTime = 14;
        v[3].arrivalTime = 9;
        v[4].arrivalTime = 16;
        v[5].arrivalTime = 21;
        v[6].arrivalTime = 26;

        v[0].burstTime = 10;
        v[1].burstTime = 3;
        v[2].burstTime = 7;
        v[3].burstTime = 5;
        v[4].burstTime = 4;
        v[5].burstTime = 6;
        v[6].burstTime = 2;

        v[0].remainingTime = v[0].burstTime;
        v[1].remainingTime = v[1].burstTime;
        v[2].remainingTime = v[2].burstTime;
        v[3].remainingTime = v[3].burstTime;
        v[4].remainingTime = v[4].burstTime;
        v[5].remainingTime = v[5].burstTime;
        v[6].remainingTime = v[6].burstTime;
    }
}

/**
 * This function initialises semaphores
 */
void initialiseSemaphore(void)
{
    if (sem_init(&sem_read, 0, 0))
    {
        perror("Error: initialising semaphore read.");
        exit(1);
    }

    if (sem_init(&sem_write, 0, 1))
    {
        perror("Error: initialising semaphore write.");
        exit(1);
    }
}

/**
 * This function unlinks and deletes FIFO
 */
void delFifo(void)
{
    // Unable to remove FIFO properly
    if (unlink(FIFONAME) < 0)
    {
        perror("Error while removing FIFO.\n");
        exit(EXIT_FAILURE);
    }

    // FIFO removed successfully on program completion
    else
    {
        printf("Program completed successfully!\n");
        exit(EXIT_SUCCESS);
    }
}

/**
 * This function creates and initialises FIFO
 */
void createFifo(void)
{
    int fdwrite;
    int checkFifo;

    // Create the FIFO (named pipe)
    checkFifo = mkfifo(FIFONAME, 0666);

    //Check if FIFO can be constructed
    if (checkFifo != 0)
    {
        perror("Error: Fifo not created!\n");
        exit(EXIT_FAILURE);
    }

    //Check if FIFO file handler can be opened
    if (fdwrite = open(FIFONAME, O_RDONLY | O_NONBLOCK) < 0)
    {
        perror("Error: while opening file!\n");
        delFifo();
        exit(EXIT_FAILURE);
    }
    else
        //if FIFO acquired successfully
        printf("FIFO acquired, writing data to thread2...\n");
    sleep(1);

    close(fdwrite);
}

/**
 * This function writes average waiting and turnaround time to FIFO
 */
void doFIFO(void)
{
    int fdwrite;

    // Open FIFO in the write mode
    if (fdwrite = open(FIFONAME, O_WRONLY) < 0)
    {
        printf("Error while opening the FIFO for write.\n");
        exit(EXIT_FAILURE);
    }

    // Write data into the FIFO
    write(fdwrite, string, strlen(string));

    // Close the FIFO
    close(fdwrite);
}

/**
 * This function writes data stored in the buffer to the output text file
 */
int writeFile(FILE *des, char *buf)
{
    // Check for an active status and see if data can be written to file
    if (status != 0)
    {
        fputs(buf, des);
        printf("\nWriting data from FIFO to file...\n%s", buf);
    }

    // Unable to obtain data to write to file
    else
    {
        printf("\nError while writing data from FIFO to file. Program will abort.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

/**
 * perform shortest remaining time first scheduling
 * v is runtime pointer
 * n is number of processes
 */
void srtfProcess(struct_v *v, int n)
{
    int smallest;
    int remain = 0;
    int i, time;
    int sum_wait = 0, sum_turnaround = 0, end;
    printf("+---------------------------------------+");
    printf("\nProcess\t|Turnaround Time| Waiting Time\n");
    printf("+---------------------------------------+");
    v[9].remainingTime = 9999;
    for (time = 0; remain != n; time++)
    {
        smallest = 9;
        for (i = 0; i <= n; i++)
        {
            if (v[i].arrivalTime <= time && v[i].remainingTime < v[smallest].remainingTime && v[i].remainingTime > 0)
            {
                smallest = i;
            }
        }

        v[smallest].remainingTime--;

        if (v[smallest].remainingTime == 0)
        {
            remain++;
            end = time + 1;

            printf("\nP[%d]\t|\t%d\t|\t%d", smallest + 1, end - v[smallest].arrivalTime, end - v[smallest].burstTime - v[smallest].arrivalTime);
            sum_wait += end - v[smallest].burstTime - v[smallest].arrivalTime;
            sum_turnaround += end - v[smallest].arrivalTime;
            status++;
        }
    }

    printf("\n\nAverage waiting time = %f\n", sum_wait * 1.0 / n);
    printf("Average Turnaround time = %f\n", sum_turnaround * 1.0 / n);

    totalWT = sum_wait * 1.0 / n;
    totalTT = sum_turnaround * 1.0 / n;
}

/**
 * thread 1 subroutine
 * Perform CPU scheduling calculate average wait time and turn around time
 * Put data into the FIFO buffer
 */
void *thread1_subroutine(struct_thread1 *data)
{
    //check if semaphore write is available
    sem_wait(data->sem_write);

    //program process and process number
    struct_v v[n];

    //initialise processes and load assignment value
    init(v, n);

    printf("initialise processes success!\n");

    //perform srtf scheduling
    srtfProcess(v, n);

    printf("scheduling success\n");

    // Initialise and create FIFO
    createFifo();

    // Send formatted output to a string
    sprintf(string,
            "Average Waiting Time = %.2f \nAverage Turnaround Time = %.2f \n",
            totalWT, totalTT);

    // Writes data to FIFO
    doFIFO();

    printf("\nthread 1 completion status !1");

    // Signal and unlock the read semaphore CHECK LATER
    sem_post(data->sem_read);
    //sem_wait(data->sem_write);

    printf("\nthread 1 completion status !2\n");

    pthread_exit(NULL);

    //printf(" not exited!\n");
}

/**
 * thread 2 subroutine
 * Read from FIFO and write the data into an output.txt file
 */
void *thread2_subroutine(struct_thread2 *data)
{
    //Check if semaphore read is reached
    sem_wait(data->sem_read);

    int n, fdwrite;
    int eof_reached = 0;
    char string[MESSLENGTH];

    //open FIFO in the read mode
    if (fdwrite = open(FIFONAME, O_RDONLY | O_NONBLOCK) < 0)
    {
        perror("Error: Could not open FIFO to read.\n");
        exit(EXIT_FAILURE);
    }

    printf("thread 2 completion status !1\n");

    //Read the incoming data from FIFO
    n = read(fdwrite, string, MESSLENGTH);

    //check if it is empty
    if (n == 0)
        printf("FIFO is empty.\n");
    //check for end of file
    else if (!eof_reached)
        writeFile(data->fp, string);
    //Exit if end of file reached
    else if (eof_reached)
        exit(EXIT_SUCCESS);

    //Inform the user for cleaning the FIFO
    printf("Cleaning the FIFO....\n");

    //close FIFO
    close(fdwrite);
    printf("Closing FIFO...\n");
    printf("thread 2 completion status !2\n");

    //Remove the FIFO
    delFifo();

    //Signal and unlock the write semaphore
    sem_post(data->sem_write);

    pthread_exit(NULL);
}

/**
 *The main function to execute the program
 */
int main(int argc, char *argv[])
{
    // if terminal arguments not equal to two
    if (argc != 2)
    {
        printf("Error: Program expecting 2 args!\n");
        return -1;
    }

    printf("program starting \n");
    // Program arg1 is the output text file
    FILE *stream = fopen(argv[1], "w");

    // Remove FIFO if it exists
    unlink(FIFONAME);

    // Check if destination file can be opened for writing
    if (!stream)
    {
        printf("Error while opening destination file for writing.\n");
        printf("Program will abort.\n");
        exit(EXIT_FAILURE);
    }

    //initialise semaphore
    initialiseSemaphore();
    printf("semaphore init success main\n");

    //initialise ans set up shared data structture
    struct_thread1 a = {&sem_write, &sem_read};
    struct_thread2 b = {&sem_read, &sem_write, stream};

    // Load program details and instructions yasin details
    instructions();

    //create the threads
    if (pthread_create(&thread1, NULL, (void *)thread1_subroutine, &a) != 0)
    {
        printf("Error while creating thread1! Program will abort.\n");
        exit(EXIT_FAILURE);
    }
    printf("thread 1 created successfully!\n");
    if (pthread_create(&thread2, NULL, (void *)thread2_subroutine, &b) != 0)
    {
        printf("Error while creating thread2! Program will abort.\n");
        exit(EXIT_FAILURE);
    }
    printf("\nthread 2 created successfully!\n");

    // Wait for the thread to exit
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // Destroy the semaphores
    sem_destroy(&sem_write);
    sem_destroy(&sem_read);

    // Check if the destination file was closed successfully
    if (!stream)
    {
        if (fclose(stream))
        {
            printf("Error while closing the destination file.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Remove the FIFO
    delFifo();

    exit(EXIT_SUCCESS);
}
